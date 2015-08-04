/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _SGCLIENT_H
#define _SGCLIENT_H

#include <string>
#include <vector>
#include <limits>
#include <unordered_map>

#include <sstream>
#include "SideGraph.h"
#include "Download.h"


/** 
All logic for reading side graph data from the GA4GH client, 
nameley Sequences, Joins and AllelePaths.  Uses libcurl 
which is left as dependency (maybe move to submodule?) for now

design is that stuff gets downloaded into a single SideGraph object
(_sg) for which SGClient is responsible for deleting. 
*/

class SGClient
{
public:

   SGClient();
   ~SGClient();

   /** free up all memory stored in side graph */
   void erase();

   /** set URL to be used by all the other methods */
   void setURL(const std::string& baseURL);

   /** print a few messages here if specified */
   void setOS(std::ostream* os);

   typedef std::pair<std::string, std::vector<SGSegment> > NamedPath;
   /** Download a whole Side Graph into memory.  Topolgy gets stored 
    * internally in (returned) SideGraph, path and bases get stored in 
    * the given vectors */
   const SideGraph* downloadGraph(std::vector<std::string>& outBases,
                                  std::vector<NamedPath>& outPaths);

   /** Download sequences into the Side Graph. returns number of sequences */
   int downloadSequences(std::vector<const SGSequence*>& outSequences,
                         int idx = 0,
                         int numSequences = std::numeric_limits<int>::max(),
                         int referenceSetID = -1,
                         int variantSetID = -1);

   /** Download the DNA bases for a given sequence.  Note the ID here is
    * the mapped ID (ie used by SideGraph class) */
   int downloadBases(sg_int_t sgSeqID, std::string& outBases, int start = 0,
                     int end = -1);

   /** Download joins into the Side Graph. returns number of joins. Note
    * must download Sequences first!!  Sequence ID's in joins are
    * automatically mapped to in-memory Side Graph ids.  */
   int downloadJoins(std::vector<const SGJoin*>& outJoins,
                     int idx = 0,
                     int numJoins = std::numeric_limits<int>::max(),
                     int referenceSetID = -1,
                     int variantSetID = -1);

   
   /** Download allele path.  returns -1 if path not found */
   int downloadAllele(int alleleID, std::vector<SGSegment>& outPath,
                      int& outVariantSetID, std::string& outName);

   /** SideGraph class, as currently implemented, only works with 
    * sequences with ids in [0, n), and it happily changes input id's 
    * to enforce this.  We therefore keep a little map to get back the
    * ID from the graph server sequence, given a side graph sequence */
   sg_int_t getOriginalSeqID(sg_int_t sgID) const;
   /** Other direction */
   sg_int_t getSGSeqID(sg_int_t sgID) const;
   /** Apply mapping (original->sg) to join */
   void mapSeqIDsInJoin(SGJoin& join) const;
   /** Add a mapping */
   void addSeqIDMapping(sg_int_t originalID, sg_int_t sgID);
   

   /** Get access to Side Graph that's been downloaded so far */
   const SideGraph* getSideGraph() const;
   
protected:
   
   /** Build the JSON string for some common options */
   std::string getPostOptions(int pageToken,
                              int pageSize,
                              int referenceSetID,
                              int variantSetID) const;

   /** Print logging messages here */
   std::ostream& os();

   static const std::string CTHeader;
   
   SideGraph* _sg;
   std::string _url;
   Download _download;
   // sucky hack: to do: fix sidegraph and lookup to let sequences
   // have arbitrary ids.
   std::unordered_map<sg_int_t, sg_int_t> _toOrigSeqId;
   std::unordered_map<sg_int_t, sg_int_t> _fromOrigSeqId;
   std::ostream* _os;
   std::stringstream _ignore;
};

inline sg_int_t SGClient::getOriginalSeqID(sg_int_t sgID) const
{
  assert(_toOrigSeqId.find(sgID) != _toOrigSeqId.end());
  return _toOrigSeqId.find(sgID)->second;
}

inline sg_int_t SGClient::getSGSeqID(sg_int_t origID) const
{
  assert(_fromOrigSeqId.find(origID) != _fromOrigSeqId.end());
  return _fromOrigSeqId.find(origID)->second;
}

inline void SGClient::mapSeqIDsInJoin(SGJoin& join) const
{
  // man, that crappy SideGraph write interface is coming to bite me. 
  join.setSide1(SGSide(SGPosition(
                         getSGSeqID(join.getSide1().getBase().getSeqID()),
                         join.getSide1().getBase().getPos()),
                       join.getSide1().getForward()));
  join.setSide2(SGSide(SGPosition(
                         getSGSeqID(join.getSide2().getBase().getSeqID()),
                         join.getSide2().getBase().getPos()),
                       join.getSide2().getForward()));
}

inline void SGClient::addSeqIDMapping(sg_int_t originalID, sg_int_t sgID)
{
  _toOrigSeqId.insert(std::pair<sg_int_t, sg_int_t>(sgID, originalID));
  _fromOrigSeqId.insert(std::pair<sg_int_t, sg_int_t>(originalID, sgID));
}

inline const SideGraph* SGClient::getSideGraph() const
{
  return _sg;
}

#endif
