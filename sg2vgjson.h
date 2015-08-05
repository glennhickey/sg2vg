/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _SG2VGJSON_H
#define _SG2VGJSON_H

#include <vector>
#include <string>
#include <stdexcept>

#include "rapidjson/document.h"

#include "sidegraph.h"

/** Handle all writing of VG JSON format here.  This should be replaced
with writing directly to VG protobuf format.  In the meantime, we can 
load it in wit vg view -J. 

Note: current implementation puts everything in memory which is quite
unncessary, but code will probably get scrapped when move to direct vg 
interface... 

We are writing a SideGraph object, but one that was created with 
side2seq -- ie all joins are to ends of sequences, so can be translated
directly to vg sequence graph... 
 
VG JSON Format follows protobuf...

node array
{"node": [{"sequence": "CAAGTAGAGGATC", "id": 1},...

"edge": [{"from": 1, "to": 2}, {"from": 1, "to": 3},

"path": [{"name": "c", "mapping": [{"position": {"node_id": 1}},

*/

class SG2VGJSON
{
public:
   SG2VGJSON();
   ~SG2VGJSON();

   /** init output stream and json document */
   void init(std::ostream* os);

   /** write nodes and edges and paths*/
   void writeGraph(const SideGraph* sg,
                   const std::vector<std::string>& bases,
                   const std::vector<std::pair<
                   std::string, std::vector<SGSegment> > >& paths);
   
protected:

   // add to json doc
   void addNode(const SGSequence* seq);
   void addEdge(const SGJoin* join);
   void addPath(const std::string& name, const std::vector<SGSegment>& path);

   // rapidjson interface seems pretty horrible but too late to switch
   // apis.  some helpers:
   void addInt(rapidjson::Value& value, const std::string& name, int v);
   void addString(rapidjson::Value& value, const std::string& name,
                  const std::string& v);
   void addBool(rapidjson::Value& value, const std::string& name, bool v);

   // access arrays
   rapidjson::Value& nodes();
   rapidjson::Value& edges();
   rapidjson::Value& paths();
   rapidjson::Document::AllocatorType& allocator();

   std::ostream* _os;
   const SideGraph* _sg;
   const std::vector<std::string>* _bases;
   const std::vector<std::pair<std::string, std::vector<SGSegment> > >* _paths;

   rapidjson::Document* _doc;
};

inline rapidjson::Value& SG2VGJSON::nodes()
{
  return (*_doc)["node"];
}

inline rapidjson::Value& SG2VGJSON::edges()
{
  return (*_doc)["edge"];
}

inline rapidjson::Value& SG2VGJSON::paths()
{
  return (*_doc)["path"];
}

inline rapidjson::Document::AllocatorType& SG2VGJSON::allocator()
{
  return _doc->GetAllocator();
}


#endif
