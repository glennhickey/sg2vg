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
                            std::vector<SGSequence*>& outSeqs)
{
  Document json;
  json.Parse(buffer);
  const Value& jsonSeqArray = json["sequences"];
  if (!jsonSeqArray.IsArray())
  {
    throw runtime_error("Error parsing JSON Sequences Array");
  }
  outSeqs.resize(jsonSeqArray.Size());

  // rapidjson uses SizeType instead of size_t.
  for (SizeType i = 0; i < jsonSeqArray.Size(); i++)
  {
    SGSequence seq = parseSequence(jsonSeqArray[i]);
    outSeqs[i] = new SGSequence(seq.getID(), seq.getLength(), seq.getName());
  }

  return outSeqs.size();
}

SGSequence JSON2SG::parseSequence(const Value& val)
{
  const Value& jsonLength = val["length"];
  // not sure why stored in string and not int
  assert(jsonLength.IsString());
  stringstream ss;
  ss << jsonLength.GetString();
  sg_int_t sgLength;
  ss >> sgLength;
  const Value& jsonID = val["id"];
  // not sure why stored in string and not int
  assert(jsonID.IsString());
  stringstream ss2;
  ss2 << jsonID.GetString();
  sg_int_t sgID;
  ss2>> sgID;
  return SGSequence(sgID, sgLength, "todo");
}

int JSON2SG::parseJoins(const char* buffer, vector<SGJoin*>& outJoins)
{
  // Read in the JSON string into Side Graph objects
  Document json;
  json.Parse(buffer);
  const Value& jsonJoinArray = json["joins"];
  if (!jsonJoinArray.IsArray())
  {
    throw runtime_error("Error parsing JSON Joins Array");
  }
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
  const Value& jsonStrand = val["strand"];
  assert(jsonStrand.IsString());
  string strand = jsonStrand.GetString();
  assert(strand == "POS_STRAND" || strand == "NEG_STRAND");
  bool forward = strand == "POS_STRAND";
  return SGSide(pos, forward);
}

SGPosition JSON2SG::parsePosition(const Value& val)
{
  assert(!val.IsNull());
  const Value& jsonPos = val["position"];
  // not sure why stored in string and not int
  assert(jsonPos.IsString());
  stringstream ss;
  ss << jsonPos.GetString();
  sg_int_t pos;
  ss >> pos;
  const Value& jsonSeqID = val["sequenceId"];
  // not sure why stored in string and not int
  assert(jsonSeqID.IsString());
  stringstream ss2;
  ss2 << jsonSeqID.GetString();
  sg_int_t seqid;
  ss2 >> seqid;
  return SGPosition(pos, seqid);
}
