/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#include <iostream>
#include <sstream>

#include "rapidjson/document.h"    
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "sgclient.h"
#include "json2sg.h"


using namespace std;
using namespace rapidjson;

const string SGClient::CTHeader = "Content-Type: application/json";

SGClient::SGClient() : _sg(0), _os(0)
{

}

SGClient::~SGClient()
{
  erase();
}

void SGClient::erase()
{
  delete _sg;
  _url = "";
  _sg = 0;
  _os = 0;
}

void SGClient::setURL(const string& baseURL)
{
  erase();
  
  _url = baseURL;
  assert(_url.length() > 1);
  if (_url.back() == '/')
  {
    _url.pop_back();
  }

  size_t lastSlash = _url.find_last_of("/");
  size_t lastV = _url.find_last_of("vV");

  if (lastV == string::npos ||
      (lastSlash != string::npos && lastV <= lastSlash) ||
      lastV + 6 < _url.length())
  {
    throw runtime_error("Version not detected at end of URL");
  }

  _sg = new SideGraph();
}

void SGClient::setOS(ostream* os)
{
  _os = os;
}

ostream& SGClient::os()
{
  return _os != NULL ? *_os : _ignore;
}

const SideGraph* SGClient::downloadGraph(vector<string>& outBases,
                                         vector<NamedPath>& outPaths)
{
  map<int, string> refIDMap;
  os() << "Downloading References...";
  downloadReferences(refIDMap);
  os() << " (" << refIDMap.size() << " references retrieved)" << endl;
  
  vector<const SGSequence*> seqs;
  os() << "Downloading Sequences...";
  downloadSequences(seqs, &refIDMap);
  os() << " (" << seqs.size() << " sequences retrieved)" << endl;
  
  vector<const SGJoin*> joins;
  os() << "Downloading Joins...";
  downloadJoins(joins);
  os() << " (" << joins.size() << " joins retrieved)" << endl;

  // download bases for every side graph sequence
  
  os() << "Downloading bases for " << _sg->getNumSequences()
       << " sequences... ";
  int basesCount = 0;
  outBases.resize(_sg->getNumSequences());
  for (int i = 0; i < outBases.size(); ++i)
  {
    downloadBases(i, outBases[i]);
    basesCount += outBases[i].length();
  }
  os() << " (" << basesCount << " bases retrieved)" << endl;

  // keep downloading paths until there aren't any.
  os() << "Downloading allele paths... ";
  outPaths.clear();
  NamedPath path;
  int variantSetID;
  int ret = 1; 
  for (int i = 0; ret > 0; ++i)
  {
    path.first.clear();
    path.second.clear();
    ret = downloadAllele(i, path.second, variantSetID, path.first);
    if (ret > 0)
    {
      outPaths.push_back(NamedPath());
      swap(outPaths.back(), path);
    }
  }
  os() << "(" << outPaths.size() << " paths retrieved)" << endl;
  
  return getSideGraph();
}

int SGClient::downloadSequences(vector<const SGSequence*>& outSequences,
                                const map<int, string>* nameIdMap,
                                int idx, int numSequences,
                                int referenceSetID, int variantSetID)
{
  outSequences.clear();
    
  string postOptions = getPostOptions(idx, numSequences, referenceSetID,
                                      variantSetID);

  string path = "/sequences/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // Parse the JSON output into a Sequences array and add it to the side graph
  JSON2SG parser;
  vector<SGSequence*> sequences;
  int ret = parser.parseSequences(result, sequences);
  if (ret == -1)
  {
    stringstream ss;
    ss << "Error: POST request for Sequences returned " << result;
    throw runtime_error(ss.str());
  }
  
  for (int i = 0; i < sequences.size(); ++i)
  {
    sg_int_t originalID = sequences[i]->getID();

    // no name in the json.  no references loaded.  so we give it a name
    // based on original id
    if (nameIdMap == NULL)
    {
      stringstream ss;
      ss << "Seq" << originalID;
      sequences[i]->setName(ss.str());
    }
    // name mapped from reference name
    else
    {
      map<int, string>::const_iterator si = nameIdMap->find(
        sequences[i]->getID());
      if (si == nameIdMap->end())
      {
        stringstream ss;
        ss << "Error: Could not find Reference with for sequence id "
           << sequences[i]->getID();
        throw runtime_error(ss.str());
      }
      sequences[i]->setName(si->second);
    }
    
    // store map to original id as Side Graph interface requires
    // ids be [0,n) which may be unessarily strict.  Too lazy right
    // now to track down SGExport code that depends on this..
    const SGSequence* addedSeq = _sg->addSequence(sequences[i]);
    addSeqIDMapping(originalID, addedSeq->getID());

    outSequences.push_back(addedSeq);
  }

  return outSequences.size();
}

int SGClient::downloadReferences(map<int, string>& outIdMap,
                                 int idx,
                                 int numReferences,
                                 int referenceSetID)
{
  outIdMap.clear();
    
  string postOptions = getReferencePostOptions(idx, numReferences,
                                               referenceSetID,
                                               vector<int>(),
                                               vector<string>(),
                                               vector<string>(),
                                               vector<string>());

  string path = "/references/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // Parse the JSON output into a Sequences array and add it to the side graph
  JSON2SG parser;
  int ret = parser.parseReferences(result, outIdMap);
  if (ret == -1)
  {
    stringstream ss;
    ss << "Error: POST request for References returned " << result;
    throw runtime_error(ss.str());
  }

  return outIdMap.size();
}

int SGClient::downloadBases(sg_int_t sgSeqID, string& outBases, int start,
                            int end)
{
  outBases.clear();

  const SGSequence* seq = _sg->getSequence(sgSeqID);
  if (seq == NULL)
  {
    stringstream ss;
    ss << "Unable to downloadBases for sgSeqID " << sgSeqID << " because"
       << " sequence was never downloaded using downloadSequecnes";
    throw runtime_error(ss.str());
  }
  int origID = getOriginalSeqID(sgSeqID);

  int queryLen = seq->getLength();
  stringstream opts;
  opts << "/sequences/" << origID << "/bases";
  if (start != 0 && end != -1)
  {
    if (start < 0 || end < 0 || end <= start || end > seq->getLength())
    {
      stringstream ss;
      ss << "start=" << start << ", end=" << end << " invalid for sequence"
         << " id=" << origID << " with length " << seq->getLength();
      throw runtime_error(ss.str());
    }
    opts << "?start=" << start << "\\&end=" << end;
    queryLen = end - start;
  }

  string path = opts.str();
  
  // Send the Request
  const char* result = _download.getRequest(_url + path,
                                            vector<string>());

  // Parse the JSON output into a string
  JSON2SG parser;
  int ret = parser.parseBases(result, outBases);
  if (ret == -1)
  {
    stringstream ss;
    ss << "Error: GET request for Bases for seqid=" << sgSeqID
       << " returned " << result;
    throw runtime_error(ss.str());
  }

  if (outBases.length() != queryLen)
  {
    stringstream ss;
    ss << "Tried to download " << queryLen << " bases for sequence "
       << origID << " but got " << outBases.length() << " bases.";
    throw runtime_error(ss.str());
  }

  return outBases.length();
}

int SGClient::downloadJoins(vector<const SGJoin*>& outJoins,
                            int idx, int numJoins,
                            int referenceSetID, int variantSetID)
{
  outJoins.clear();
  
  string postOptions = getPostOptions(idx, numJoins, referenceSetID,
                                      variantSetID);
  string path = "/joins/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // Parse the JSON output into a Joins array and add it to the side graph
  JSON2SG parser;
  vector<SGJoin*> joins;
  int ret = parser.parseJoins(result, joins);
  if (ret == -1)
  {
    stringstream ss;
    ss << "Error: POST request for Joins returned " << result;
    throw runtime_error(ss.str());
  }
  
  for (int i = 0; i < joins.size(); ++i)
  {
    mapSeqIDsInJoin(*joins[i]);
    outJoins.push_back(_sg->addJoin(joins[i]));
  }

  return outJoins.size();
}

int SGClient::downloadAllele(int alleleID, vector<SGSegment>& outPath,
                             int& outVariantSetID, string& outName)
{
  outPath.clear();

  stringstream opts;
  opts << "/alleles/" << alleleID;
  string path = opts.str();

  const char* result = _download.getRequest(_url + path,
                                            vector<string>());

  int outID;
  JSON2SG parser;
  int ret = parser.parseAllele(result, outID, outPath, outVariantSetID,
                               outName);

  if (ret >=0 && outID != alleleID)
  {
    throw runtime_error("AlleleID mismatch");
  }

  return ret;
}

string SGClient::getPostOptions(int pageToken,
                                int pageSize,
                                int referenceSetID,
                                int variantSetID) const
{
   // Build JSON POST Options
  Document doc;
  doc.Parse("{}");
  Value nv;
  assert(nv.IsNull());
  doc.AddMember("pageSize", nv, doc.GetAllocator());
  doc["pageSize"].SetInt64(pageSize);
  doc.AddMember("pageToken", nv, doc.GetAllocator());
  if (pageToken > 0)
  {
    doc["pageToken"].SetInt64(pageToken);
  }
  doc.AddMember("referenceSetId", nv, doc.GetAllocator());
  if (referenceSetID >= 0)
  {
    doc["referenceSetId"].SetInt64(referenceSetID);
  }
  doc.AddMember("variantSetId", nv, doc.GetAllocator());
  if (variantSetID >= 0)
  {
    doc["variantSetId"].SetInt64(variantSetID);
  }
  
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

string SGClient::getReferencePostOptions(int pageToken,
                                         int pageSize,
                                         int referenceSetID,
                                         const vector<int>& seqIDs,
                                         const vector<string>& md5s,
                                         const vector<string>& accs,
                                         const vector<string>& rnames) const
{
  // Build JSON POST Options
  Document doc;
  doc.Parse("{}");
  Value nv;
  assert(nv.IsNull());
  doc.AddMember("pageSize", nv, doc.GetAllocator());
  doc["pageSize"].SetInt64(pageSize);
  doc.AddMember("pageToken", nv, doc.GetAllocator());
  if (pageToken > 0)
  {
    doc["pageToken"].SetInt64(pageToken);
  }
  doc.AddMember("referenceSetId", nv, doc.GetAllocator());
  if (referenceSetID >= 0)
  {
    doc["referenceSetId"].SetInt64(referenceSetID);
  }

  if (seqIDs.size() > 0 ||
      md5s.size() > 0 ||
      accs.size() > 0 ||
      rnames.size() > 0)
  {
    throw runtime_error("NOT IMPLEMENTED");
  }

  Value jsonIds;
  jsonIds.SetArray();
  doc.AddMember("sequenceIds", jsonIds, doc.GetAllocator());
  
  Value jsonMd5s;
  jsonMd5s.SetArray();
  doc.AddMember("md5checksums", jsonMd5s, doc.GetAllocator());

  Value jsonAccessions;
  jsonAccessions.SetArray();
  doc.AddMember("accessions", jsonAccessions, doc.GetAllocator());

  Value jsonNames;
  jsonNames.SetArray();
  doc.AddMember("referenceNames", jsonNames, doc.GetAllocator());
  
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}
  
