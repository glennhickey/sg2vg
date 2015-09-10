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

const int SGClient::DefaultPageSize = 1000;
const string SGClient::CTHeader = "Content-Type: application/json";

SGClient::SGClient() : _sg(0), _os(0), _pageSize(DefaultPageSize)
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
  if (_url[_url.length() - 1] == '/')
  {
    _url.resize(_url.length() - 1);
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

void SGClient::setPageSize(int pageSize)
{
  _pageSize = pageSize;
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
  for (int pageToken = 0; pageToken >= 0;)
  {
    pageToken = downloadReferences(refIDMap, pageToken, _pageSize);
  }
  os() << " (" << refIDMap.size() << " references retrieved)" << endl;
  if (refIDMap.size() == 0)
  {
    os() << "Warning: No references found" << endl;
  }
  
  vector<const SGSequence*> seqs;
  outBases.clear();
  os() << "Downloading Sequences...";
  for (int pageToken = 0; pageToken >= 0;)
  {
    pageToken = downloadSequences(seqs, &outBases,
                                  refIDMap.empty() ? NULL : &refIDMap,
                                  pageToken, _pageSize);
  }
  os() << " (" << seqs.size() << " sequences retrieved)" << endl;
  
  vector<const SGJoin*> joins;
  os() << "Downloading Joins...";
  for (int pageToken = 0; pageToken >= 0;)
  {
    pageToken = downloadJoins(joins, pageToken, _pageSize);
  }
  os() << " (" << joins.size() << " joins retrieved)" << endl;


  outPaths.clear();
  os() << "Downloading allele paths... ";
  for (int pageToken = 0; pageToken >= 0;)
  {
    pageToken = downloadAllelePaths(outPaths, pageToken, _pageSize);
  }
  os() << "(" << outPaths.size() << " paths retrieved)" << endl;
  
  return getSideGraph();
}

int SGClient::downloadSequences(vector<const SGSequence*>& outSequences,
                                vector<string>* outBases,
                                const map<int, string>* nameIdMap,
                                int pageToken, int pageSize,
                                int referenceSetID, int variantSetID)
{    
  string postOptions = getSequencePostOptions(pageToken, pageSize,
                                              referenceSetID,
                                              variantSetID, outBases != NULL);

  string path = "/sequences/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // Parse the JSON output into a Sequences array and add it to the side graph
  JSON2SG parser;
  vector<SGSequence*> sequences;
  vector<string> bases;
  int nextPageToken = -2;
  int ret = parser.parseSequences(result, sequences, bases, nextPageToken);
  if (ret == -1 || nextPageToken <= -2)
  {
    stringstream ss;
    ss << "Error: POST request for Sequences returned " << result;
    throw runtime_error(ss.str());
  }
  if (nextPageToken >= 0 && pageToken + sequences.size() != nextPageToken)
  {
    stringstream ss;
    ss << "Error: nextPageToken=" << nextPageToken << " returned does not "
       << "equal number of sequences returned (" << sequences.size()
       << ") + pageToken=" << pageToken;
    throw runtime_error(ss.str());
  }
  
  assert(!outBases || bases.size() == sequences.size());
  
  for (int i = 0; i < sequences.size(); ++i)
  {
    sg_int_t originalID = sequences[i]->getID();

    bool foundName = false;
    if (nameIdMap != NULL)
    {
      map<int, string>::const_iterator si = nameIdMap->find(
        sequences[i]->getID());
      if (si != nameIdMap->end())
      {
        // name mapped from reference name
        sequences[i]->setName(si->second);
        foundName = true;
      }
      else
      {
        os() << "\nWarning: Could not find Reference for sequence id "
             << sequences[i]->getID() << " ";
      }
    }

    // no name in the json.  no reference found / loaded.  so we give it a name
    // based on original id
    if (foundName == false)
    {
      stringstream ss;
      ss << "Seq" << originalID;
      sequences[i]->setName(ss.str());
    }
    
    // store map to original id as Side Graph interface requires
    // ids be [0,n) which may be unessarily strict.  Too lazy right
    // now to track down SGExport code that depends on this..
    const SGSequence* addedSeq = _sg->addSequence(sequences[i]);
    addSeqIDMapping(originalID, addedSeq->getID());

    outSequences.push_back(addedSeq);
    if (outBases != NULL)
    {
      outBases->push_back(bases[i]);
    }
  }

  return nextPageToken;
}

int SGClient::downloadReferences(map<int, string>& outIdMap,
                                 int pageToken,
                                 int pageSize,
                                 int referenceSetID)
{   
  string postOptions = getReferencePostOptions(pageToken, pageSize,
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
  map<int, string> idMap;
  int nextPageToken = -2;
  int ret = parser.parseReferences(result, idMap, nextPageToken);
  if (ret == -1 || nextPageToken <= -2)
  {
    stringstream ss;
    ss << "Error: POST request for References returned " << result;
    throw runtime_error(ss.str());
  }
  if (nextPageToken >= 0 && pageToken + idMap.size() != nextPageToken)
  {
    stringstream ss;
    ss << "Error: nextPageToken=" << nextPageToken << " returned does not "
       << "equal number of references returned (" << idMap.size()
       << ") + pageToken=" << pageToken;
    throw runtime_error(ss.str());
  }

  outIdMap.insert(idMap.begin(), idMap.end());

  return nextPageToken;
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
                            int pageToken, int pageSize,
                            int referenceSetID, int variantSetID)
{ 
  string postOptions = getJoinPostOptions(pageToken, pageSize, referenceSetID,
                                          variantSetID);
  string path = "/joins/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // Parse the JSON output into a Joins array and add it to the side graph
  JSON2SG parser;
  vector<SGJoin*> joins;
  int nextPageToken = -2;
  int ret = parser.parseJoins(result, joins, nextPageToken);
  if (ret == -1 || nextPageToken <= -2)
  {
    stringstream ss;
    ss << "Error: POST request for Joins returned " << result;
    throw runtime_error(ss.str());
  }
  if (nextPageToken >= 0 && pageToken + joins.size() != nextPageToken)
  {
    stringstream ss;
    ss << "Error: nextPageToken=" << nextPageToken << " returned does not "
       << "equal number of joins returned (" << joins.size()
       << ") + pageToken=" << pageToken;
    throw runtime_error(ss.str());
  }
  
  for (int i = 0; i < joins.size(); ++i)
  {
    verifyInJoin(*joins[i]);
    mapSeqIDsInJoin(*joins[i]);
    outJoins.push_back(_sg->addJoin(joins[i]));
  }

  return nextPageToken;
}

int SGClient::downloadAllelePaths(vector<NamedPath>& outPaths,
                                  int pageToken, int pageSize,
                                  int sequenceID,
                                  const vector<int>* variantSetIDs,
                                  int start, int end)
{
  string postOptions = getAllelePostOptions(pageToken, pageSize, sequenceID,
                                            variantSetIDs, start, end);
  string path = "/alleles/search";

  // Send the Request
  const char* result = _download.postRequest(_url + path,
                                             vector<string>(1, CTHeader),
                                             postOptions);

  // POST Request doesn't return paths for some reason.  So we scrape out
  // all the allele ID's from the result:
  JSON2SG parser;
  vector<int> alleleIDs;
  int nextPageToken = -2;
  int ret = parser.parseAlleleIDs(result, alleleIDs, nextPageToken);
  if (ret == -1 || nextPageToken <= -2)
  {
    stringstream ss;
    ss << "Error: POST request for Alleles returned " << result;
    throw runtime_error(ss.str());
  }
  if (nextPageToken >= 0 && pageToken + alleleIDs.size() != nextPageToken)
  {
    stringstream ss;
    ss << "Error: nextPageToken=" << nextPageToken << " returned does not "
       << "equal number of alleles returned (" << alleleIDs.size()
       << ") + pageToken=" << pageToken;
    throw runtime_error(ss.str());
  }

  // With IDs in hand, we call downloadAllele on each one to get the path.
  NamedPath allelePath;
  int alleleVariantSetID;
  for (int i = 0; i < alleleIDs.size(); ++i)
  {
    allelePath.first.clear();
    allelePath.second.clear();
    ret = downloadAllele(alleleIDs[i], allelePath.second, alleleVariantSetID,
                         allelePath.first);
    outPaths.push_back(NamedPath());
    swap(outPaths.back(), allelePath);
  }

  return nextPageToken;
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

  verifyInPath(alleleID, outPath);

  return ret;
}

void SGClient::verifyInJoin(const SGJoin& join) const
{
  const SGPosition& pos1 = join.getSide1().getBase();
  const SGPosition& pos2 = join.getSide2().getBase();
  sg_int_t sgid1 = getSGSeqID(pos1.getSeqID());
  sg_int_t sgid2 = getSGSeqID(pos2.getSeqID());

  if (sgid1 < 0 || sgid2 < 0)
  {
    stringstream ss;
    ss << "Invalid input join: " << join << ". Sequence ID "
       << (sgid1 < 0 ? pos1.getSeqID() : pos2.getSeqID()) 
       << " not found in input graph";
    throw runtime_error(ss.str());
  }

  const SGSequence* seq1 = _sg->getSequence(sgid1);
  const SGSequence* seq2 = _sg->getSequence(sgid2);

  if (pos1.getPos() < 0 || pos1.getPos() >= seq1->getLength())
  {
    stringstream ss;
    ss << "Invalid input join: " << join << ". Position of Side 1"
       << " (" << pos1.getPos() << ") "
       << "not within Sequence with ID=" << pos1.getSeqID()
       << " which has length=" << seq1->getLength();
    throw runtime_error(ss.str());
  }

  if (pos2.getPos() < 0 || pos2.getPos() >= seq2->getLength())
  {
    stringstream ss;
    ss << "Invalid input join: " << join << ". Position of Side 2"
       << " (" << pos2.getPos() << ") "
       << "not within Sequence with ID=" << pos2.getSeqID()
       << " which has length=" << seq2->getLength();
    throw runtime_error(ss.str());
  }
}

void SGClient::verifyInPath(int alleleID, const vector<SGSegment>& path) const
{
  for (int i = 0; i < path.size(); ++i)
  {
    const SGSegment& seg = path[i];
    const SGPosition& pos = seg.getSide().getBase();
    sg_int_t sgid = getSGSeqID(pos.getSeqID());
    if (sgid < 0)
    {
      stringstream ss;
      ss << "Segment " << i << " of allele path " << alleleID
         << " has unknown sequence ID " << pos.getSeqID();
      throw runtime_error(ss.str());
    }
    
    const SGSequence* seq = _sg->getSequence(sgid);

    if (seg.getMinPos().getPos() < 0 ||
        seg.getMaxPos().getPos() >= seq->getLength())
    {
      stringstream ss;
      ss << "Segment " << i << " of allele path " << alleleID
         << " runs from " << seg.getInSide().getBase().getPos() << " to "
         << seg.getOutSide().getBase().getPos() << ", inclusive."
         << " This range is invalid as it spans bases not in "
         << "sequence with ID=" << pos.getSeqID() << " and length="
         << seq->getLength();
      throw runtime_error(ss.str());      
    }
  }
}

string SGClient::getSequencePostOptions(int pageToken,
                                        int pageSize,
                                        int referenceSetID,
                                        int variantSetID,
                                        bool getBases) const
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
    stringstream ss;
    ss << pageToken;
    doc["pageToken"].SetString(ss.str().c_str(), ss.str().length(),
                               doc.GetAllocator());
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
  Value listBases;
  listBases.SetBool(getBases);
  doc.AddMember("listBases", listBases, doc.GetAllocator());
  
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
    stringstream ss;
    ss << pageToken;
    doc["pageToken"].SetString(ss.str().c_str(), ss.str().length(),
                               doc.GetAllocator());
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

string SGClient::getJoinPostOptions(int pageToken,
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
    stringstream ss;
    ss << pageToken;
    doc["pageToken"].SetString(ss.str().c_str(), ss.str().length(),
                               doc.GetAllocator());
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

string SGClient::getAllelePostOptions(int pageToken,
                                      int pageSize,
                                      int sequenceID,
                                      const vector<int>* variantSetIDs,
                                      int start,
                                      int end) const
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
    stringstream ss;
    ss << pageToken;
    doc["pageToken"].SetString(ss.str().c_str(), ss.str().length(),
                               doc.GetAllocator());
  }

  // default to empty string (if -1) unlike pagetoken which defaults to null.
  Value jsonSequenceID;
  string sequenceIDString;
  if (sequenceID >= 0)
  {
    stringstream ss;
    ss << sequenceID;
    sequenceIDString = ss.str();
  }
  jsonSequenceID.SetString(sequenceIDString.c_str(), sequenceIDString.length(),
                           doc.GetAllocator());
  doc.AddMember("sequenceId", jsonSequenceID, doc.GetAllocator());

  Value jsonVariantSetIDs;
  jsonVariantSetIDs.SetArray();
  doc.AddMember("variantSetIds", jsonVariantSetIDs, doc.GetAllocator());
  if (variantSetIDs != NULL)
  {
    for (int i = 0; i < variantSetIDs->size(); ++i)
    {
      Value id;
      id.SetInt64(variantSetIDs->at(i));
      jsonVariantSetIDs.PushBack(id, doc.GetAllocator());
    }
  }
  
  doc.AddMember("start", nv, doc.GetAllocator());
  doc["start"].SetInt64(start);

  doc.AddMember("end", nv, doc.GetAllocator());
  doc["end"].SetInt64(end);
 
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}
