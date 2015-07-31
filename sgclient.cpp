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

SGClient::SGClient() : _sg(0)
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
}

void SGClient::setURL(const string& baseURL)
{
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
}

int SGClient::downloadSequences(vector<const SGSequence*>& outSequences,
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
  parser.parseSequences(result, sequences);
  for (int i = 0; i < sequences.size(); ++i)
  {
    // store map to original id as Side Graph interface requires
    // ids be [0,n) which may be unessarily strict.  Too lazy right
    // now to track down SGExport code that depends on this..
    sg_int_t originalID = sequences[i]->getID();
    const SGSequence* addedSeq = _sg->addSequence(sequences[i]);
    addSeqIDMapping(originalID, addedSeq->getID());
    outSequences.push_back(addedSeq);
  }

  return outSequences.size();
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
  parser.parseJoins(result, joins);
  for (int i = 0; i < joins.size(); ++i)
  {
    mapSeqIDsInJoin(*joins[i]);
    outJoins.push_back(_sg->addJoin(joins[i]));
  }

  return outJoins.size();
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
