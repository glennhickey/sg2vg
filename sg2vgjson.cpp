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
                           const vector<SGNamedPath>& paths)
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

void SG2VGJSON::writeChunkedGraph(const SideGraph* sg,
                                  const std::vector<std::string>& bases,
                                  const std::vector<SGNamedPath>& paths,
                                  int sequencesPerChunk,
                                  int joinsPerChunk,
                                  int pathSegsPerChunk)
{
  // current offset wrt to what's been written so far
  int sequencesAdded = 0;
  SideGraph::JoinSet::const_iterator joinsAdded = sg->getJoinSet()->begin();
  int pathsAdded = 0;
  int segmentsAdded = 0; // note this is relative to current last path. 

  for(bool wroteSomething = true; wroteSomething == true;)
  {
    // current count in chunk
    int sequencesInChunk = 0;
    int joinsInChunk = 0;
    int segmentsInChunk = 0;

    init(_os);

    _sg = sg;
    _bases = &bases;
    _paths = &paths;

    *_os << "{";
  
    // add every node to json doc
    for (; sequencesInChunk < sequencesPerChunk &&
            sequencesAdded < _sg->getNumSequences();
         ++sequencesInChunk, ++sequencesAdded)
    {
      addNode(_sg->getSequence(sequencesAdded));
    }
    double progress = (double)sequencesInChunk / sequencesPerChunk;
    
    // add every edge to json doc
    int joinsToAdd = joinsPerChunk * (1. - progress);
    const SideGraph::JoinSet* joinSet = _sg->getJoinSet();
    for (; joinsAdded != joinSet->end() && joinsInChunk < joinsToAdd;
         ++joinsInChunk, ++joinsAdded)
    {
      addEdge(*joinsAdded);
    }
    progress += (double)joinsInChunk / joinsPerChunk;

    // add every path to json doc
    int segmentsToAdd = pathSegsPerChunk * (1. - progress);
    for (; pathsAdded < paths.size() && segmentsInChunk < segmentsToAdd;
         ++pathsAdded)
    {
      const SGNamedPath& path = paths[pathsAdded];
      if (segmentsAdded == 0 && segmentsToAdd >= path.second.size())
      {
        // we're adding entire path
        addPath(path.first, path.second);
        segmentsInChunk += path.second.size();
      }
      else
      {
        // we need to chop path
        while (segmentsAdded < path.second.size() &&
               segmentsInChunk < segmentsToAdd)
        {
          int pathChunkSize = min(pathSegsPerChunk,
                                  (int)path.second.size() - segmentsAdded);
          vector<SGSegment>::const_iterator a =
             path.second.begin() + segmentsAdded;
          vector<SGSegment>::const_iterator b = a + pathChunkSize;
          addPath(path.first, vector<SGSegment>(a, b), segmentsAdded);
          segmentsAdded += pathChunkSize;
          segmentsInChunk += pathChunkSize;
        }
      }
      segmentsAdded = 0;
    }

    // after this we'll have added 2 copies of the graph to memory
    // 1) json doc 2) this buffer 
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    _doc->Accept(writer);
    // make that a 3rd time -- strip outer nesting {}'s.
    string temp(buffer.GetString());
    *_os << temp.substr(1, temp.length()-1);

    wroteSomething = sequencesInChunk + joinsInChunk + segmentsInChunk > 0;
  }
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
  addBool(edge, "from_start", join->getSide1().getForward() == true);
  addBool(edge, "to_end", join->getSide2().getForward() == false);
  edges().PushBack(edge, allocator());
}

void SG2VGJSON::addPath(const string& name, const vector<SGSegment>& path,
                        int rank)
{
  Value jpath;
  jpath.SetObject();
  addString(jpath, "name", name);
  Value mappings;
  mappings.SetArray();
  int inputPathLength = 0;
  int outputPathLength = 0;
  for (int i = 0; i < path.size(); ++i)
  {
    sg_int_t sgSeqID = path[i].getSide().getBase().getSeqID();
    
    if (path[i].getLength() != _sg->getSequence(sgSeqID)->getLength())
    {
      stringstream ss;
      ss << "Sanity check fail for Mapping " << i << " of path " << name
         << ": Segment size " << path[i].getLength() << " does not span "
         << "all of node " << (sgSeqID + 1) << " which has length "
         << _sg->getSequence(sgSeqID)->getLength();
      throw runtime_error(ss.str());
    }
    inputPathLength += path[i].getLength();
    
    Value position;
    position.SetObject();
    // node id's are 1-based in VG!
    addInt(position, "node_id", sgSeqID + 1);
    // Offsets are along the strand of the node that is being visited.
    // We always use the whole node.
    addInt(position, "offset", 0);
    
    addBool(position, "is_reverse", !path[i].getSide().getForward());
    
    outputPathLength += _sg->getSequence(sgSeqID)->getLength();    
    Value mapping;
    mapping.SetObject();
    mapping.AddMember("position", position, allocator());
    addInt(mapping, "rank", rank + i + 1);
    mappings.PushBack(mapping, allocator());
  }
  if (inputPathLength != outputPathLength)
  {
    stringstream ss;
    ss << "Sanity check fail for path " << name << ": input length ("
       << inputPathLength << ") != output length (" << outputPathLength << ")";
    throw runtime_error(ss.str());
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
