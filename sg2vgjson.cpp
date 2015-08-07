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

#include "sg2vgjson.h"

using namespace std;
using namespace rapidjson;

SG2VGJSON::SG2VGJSON() : _os(0), _sg(0), _bases(0), _paths(0), _doc(0)
{
}

SG2VGJSON::~SG2VGJSON()
{
  delete _doc;
}

void SG2VGJSON::init(ostream* os)
{
  _os = os;
  delete _doc;

  _doc = new Document();
  _doc->Parse("{\"node\": [], \"edge\": [], \"path\": []}");
  assert(_doc->IsObject());
  assert(nodes().IsArray());
  assert(edges().IsArray());
  assert(paths().IsArray());
}

void SG2VGJSON::writeGraph(const SideGraph* sg,
                           const vector<string>& bases,
                           const vector<pair<string, vector<SGSegment> > >&
                           paths)
{
  _sg = sg;
  _bases = &bases;
  _paths = &paths;

  *_os << "{";
  
  // add every node to json doc
  for (int i = 0; i < _sg->getNumSequences(); ++i)
  {
    addNode(_sg->getSequence(i));
  }

  // add every edge to json doc
  const SideGraph::JoinSet* joinSet = _sg->getJoinSet();
  for (SideGraph::JoinSet::const_iterator i = joinSet->begin();
       i != joinSet->end(); ++i)
  {
    addEdge(*i);
  }

  // add every path to json doc
  for (int i = 0; i < paths.size(); ++i)
  {
    addPath(paths[i].first, paths[i].second);
  }

  // after this we'll have added 2 copies of the graph to memory
  // 1) json doc 2) this buffer 
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  _doc->Accept(writer);
  // make that a 3rd time -- strip outer nesting {}'s.
  string temp(buffer.GetString());
  *_os << temp.substr(1, temp.length()-1);
}

void SG2VGJSON::addNode(const SGSequence* seq)
{  
  Value node;
  node.SetObject();
  addString(node, "sequence", _bases->at(seq->getID()));
  addString(node, "name", seq->getName());
  // node id's are 1-based in VG! 
  addInt(node, "id", seq->getID() + 1);
  nodes().PushBack(node, allocator());
}

void SG2VGJSON::addEdge(const SGJoin* join)
{
  Value edge;
  edge.SetObject();
  // node id's are 1-based in VG! 
  addInt(edge, "from", join->getSide1().getBase().getSeqID() + 1);
  addInt(edge, "to", join->getSide2().getBase().getSeqID() + 1);
  addBool(edge, "from_start", join->getSide1().getBase().getPos() == 0);
  addBool(edge, "to_end", join->getSide2().getBase().getPos() != 0);
  edges().PushBack(edge, allocator());

  // VG edges are directed.  Side GRaph edges are not.  so we add other dir
  if (join->getSide1().getBase().getSeqID() !=
      join->getSide2().getBase().getSeqID())
  {
    Value edge;
    edge.SetObject();
    // node id's are 1-based in VG! 
    addInt(edge, "from", join->getSide2().getBase().getSeqID() + 1);
    addInt(edge, "to", join->getSide1().getBase().getSeqID() + 1);
    addBool(edge, "from_start", join->getSide2().getBase().getPos() == 0);
    addBool(edge, "to_end", join->getSide1().getBase().getPos() != 0);
    edges().PushBack(edge, allocator());
  }
}

void SG2VGJSON::addPath(const string& name, const vector<SGSegment>& path)
{
  Value jpath;
  jpath.SetObject();
  addString(jpath, "name", name);
  Value mappings;
  mappings.SetArray();
  for (int i = 0; i < path.size(); ++i)
  {
    Value position;
    position.SetObject();
    // node id's are 1-based in VG! 
    addInt(position, "node_id", path[i].getSide().getBase().getSeqID() + 1);
    addInt(position, "offset", 0);
    
    Value mapping;
    mapping.SetObject();
    mapping.AddMember("position", position, allocator());
    addBool(mapping, "is_reverse", !path[i].getSide().getForward());
    mappings.PushBack(mapping, allocator());
  }
  jpath.AddMember("mapping", mappings, allocator());
  paths().PushBack(jpath, allocator());
}


void SG2VGJSON::addInt(Value& value, const string& name, int v)
{
  Value intValue;
  intValue.SetInt(v);
  Value nameValue;
  nameValue.SetString(name.c_str(), name.length(), allocator());
  value.AddMember(nameValue, intValue, allocator());
}

void SG2VGJSON::addString(Value& value, const string& name, const string& v)
{
  Value stringValue;
  stringValue.SetString(v.c_str(), v.length(), allocator());
  Value nameValue;
  nameValue.SetString(name.c_str(), name.length(), allocator());
  value.AddMember(nameValue, stringValue, allocator());
}

void SG2VGJSON::addBool(Value& value, const string& name, bool v)
{
  Value boolValue;
  boolValue.SetBool(v);
  Value nameValue;
  nameValue.SetString(name.c_str(), name.length(), allocator());
  value.AddMember(nameValue, boolValue, allocator());
}
