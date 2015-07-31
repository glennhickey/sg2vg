/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#include <iostream>
#include <sstream>

#include "Browser.hpp"
#include "rapidjson/document.h"    
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "sgclient.h"

const string SGClient::CTHeader = "Content-Type: application/json"

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
  _url = baseURL + "/" + version + "/"
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
    doc.addMember(variatnSetID);
  }
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  options.Accept(writer);
  postOptions = buffer.GetString();

  Browser browser;
  vector<string> headers(1, CTHeader);
  
  string request = ContentType + " -X POST -d\'" + postOptions + "\'";

  
  
}




using namespace std;

