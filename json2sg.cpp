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

#include "json2sg.h"

using namespace std;
using namespace rapidjson;

JSON2SG::JSON2SG()
{

}

JSON2SG::~JSON2SG()
{
  
}

int JSON2SG::parseSequences(const char* buffer,
                            vector<SGSequence*>& outSeqs,
                            vector<string>& outBases,
                            int& outNextPageToken)
{
  outSeqs.clear();
  outBases.clear();
  Document json;
  json.Parse(buffer);
  outNextPageToken = getNextPageToken(json);
  if (!json.HasMember("sequences"))
  {
    return -1;
  }
  const Value& jsonSeqArray = json["sequences"];
  outSeqs.resize(jsonSeqArray.Size());
  outBases.resize(outSeqs.size());

  // rapidjson uses SizeType instead of size_t.
  for (SizeType i = 0; i < jsonSeqArray.Size(); i++)
  {
    SGSequence seq = parseSequence(jsonSeqArray[i]);
    outSeqs[i] = new SGSequence(seq.getID(), seq.getLength(), seq.getName());
    if (!jsonSeqArray[i].HasMember("bases"))
    {
      throw std::runtime_error(std::string("Error parsing JSON field: bases"));
    }
    if (!jsonSeqArray[i]["bases"].IsNull())
    {
      outBases[i] = extractStringVal<string>(jsonSeqArray[i], "bases");
    }
  }
  return outSeqs.size();
}

SGSequence JSON2SG::parseSequence(const Value& val)
{
  sg_int_t sgLength = extractStringVal<sg_int_t>(val, "length");
  sg_int_t sgID = extractStringVal<sg_int_t>(val, "id");
  // note: we don't seem to have a name field in the json
  return SGSequence(sgID, sgLength, "");
}

int JSON2SG::parseReferences(const char* buffer,
                             map<int, string>& outMap,
                             int& outNextPageToken)
{
  outMap.clear();
  Document json;
  json.Parse(buffer);
  outNextPageToken = getNextPageToken(json);
  if (!json.HasMember("references"))
  {
    return -1;
  }
  const Value& jsonRefs = json["references"];
  assert(jsonRefs.IsArray());
  for (SizeType i = 0; i < jsonRefs.Size(); i++)
  {
    const Value& refVal = jsonRefs[i];
    string name = extractStringVal<string>(refVal, "name");
    int id = extractStringVal<int>(refVal, "sequenceId");
    outMap.insert(pair<int, string>(id, name));
  }
  return outMap.size();
}

int JSON2SG::parseBases(const char* buffer, string& outBases)
{
  outBases.clear();
  Document json;
  json.Parse(buffer);
  if (!json.HasMember("sequence"))
  {
    return -1;
  }
  const Value& jsonSeq = json["sequence"];
  outBases = jsonSeq.GetString();
  return outBases.size();
}

int JSON2SG::parseJoins(const char* buffer, vector<SGJoin*>& outJoins,
                        int& outNextPageToken)
{
  outJoins.clear();
  // Read in the JSON string into Side Graph objects
  Document json;
  json.Parse(buffer);
  outNextPageToken = getNextPageToken(json);
  if (!json.HasMember("joins"))
  {
    return -1;
  }
  const Value& jsonJoinArray = json["joins"];
  outJoins.resize(jsonJoinArray.Size());
  
  // rapidjson uses SizeType instead of size_t.
  for (SizeType i = 0; i < jsonJoinArray.Size(); i++)
  {
    SGSide sgSide1 = parseSide(jsonJoinArray[i]["side1"]);
    SGSide sgSide2 = parseSide(jsonJoinArray[i]["side2"]);
    SGJoin* sgJoin = new SGJoin(sgSide1, sgSide2);
    outJoins[i] = sgJoin;
  }
  return outJoins.size();
}

SGSide JSON2SG::parseSide(const Value& val)
{
  assert(!val.IsNull());
  SGPosition pos = parsePosition(val["base"]);
  string strand = extractStringVal<string>(val, "strand");
  assert(strand == "POS_STRAND" || strand == "NEG_STRAND");
  bool forward = strand == "POS_STRAND";
  return SGSide(pos, forward);
}

SGPosition JSON2SG::parsePosition(const Value& val)
{
  sg_int_t pos = extractStringVal<sg_int_t>(val, "position");
  sg_int_t seqid = extractStringVal<sg_int_t>(val, "sequenceId");
  return SGPosition(seqid, pos);
}

int JSON2SG::parseAllele(const char* buffer, int& outID,
                         vector<SGSegment>& outPath,
                         int& outVariantSetID, string& outName)
{
  outPath.clear();  
  Document json;
  json.Parse(buffer);

  if (!json.HasMember("name") ||
      !json.HasMember("path") ||
      !json.HasMember("id") ||
      !json.HasMember("variantSetId"))
  {
    return -1;
  }

  outID = extractStringVal<int>(json, "id");
         
  const Value& jsonPath = json["path"];
  parseAllelePath(jsonPath, outPath);

  outVariantSetID = extractStringVal<int>(json, "variantSetId");
  outName = extractStringVal<string>(json, "name");

  return outPath.size();
}

int JSON2SG::parseAllelePath(const rapidjson::Value& val,
                             vector<SGSegment>& outPath)
{
  assert(val.HasMember("segments"));
  const Value& jsonSegmentArray = val["segments"];
  outPath.resize(jsonSegmentArray.Size());
  
  // rapidjson uses SizeType instead of size_t.
  for (SizeType i = 0; i < jsonSegmentArray.Size(); i++)
  {
    SGSegment sgSeg = parseSegment(jsonSegmentArray[i]);
    outPath[i] = sgSeg;
  }
  return outPath.size();
}

SGSegment JSON2SG::parseSegment(const Value& val)
{
  assert(val.HasMember("start"));
  assert(val.HasMember("length"));

  SGSide sgSide = parseSide(val["start"]);
  int length = extractStringVal<int>(val, "length");
  return SGSegment(sgSide, length);
}

int JSON2SG::getNextPageToken(const Value& val)
{
  if (val.HasMember("nextPageToken"))
  {
    const Value& npt = val["nextPageToken"];
    if (npt.IsNull())
    {
      return -1;
    }
    return extractStringVal<int>(val, "nextPageToken");
  }
  return -2;
}
