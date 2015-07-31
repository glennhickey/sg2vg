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

void SGClient::setURL(const string& baseURL, const string& version)
{
  _url = baseURL + "/" + version + "/";
}


int SGClient::downloadJoins(vector<const SGJoin*>& outJoins,
                            int idx, int numJoins,
                            int referenceSetID, int variantSetID)
{
  Document doc;
  doc.addMember("pageSize", numJoins);
  doc.addMember("pageToken");
  if (idx >=0)
  {
    doc["pageToken"].setValue(idx);
  }
  doc.addMember("referenceSetId");
  if (referenceSetID >= 0)
  {
    doc["referenceSetId"].setValue(referenceSetID);
  }
  doc.addMember("variantSetId");
  if (variantSetID >= 0)
  {
    doc.addMember(variantSetID);
  }
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  string postOptions = buffer.GetString();

}

