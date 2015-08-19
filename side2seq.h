/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _SIDE2SEQ_H
#define _SIDE2SEQ_H

#include <string>
#include <vector>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include "sidegraph.h"
#include "sglookup.h"

/** 
Core logic for converting a Side Graph into a Sequence Graph.  

The key difference between the two is that Sequence Graph joins must be at 
ends of nodes. 

We use the SideGraph class as both input and output. 
*/

class Side2Seq
{
public:

   Side2Seq();
   ~Side2Seq();

   void reset();

   // Needs to be same as one in SGClient (but do separately since want this
   // class to stand alone from SGClient for some reason)
   typedef std::pair<std::string, std::vector<SGSegment> > NamedPath;

   /** Load of a SideGraph, some bases, and some paths.  note we do not 
    * copy anything*/
   void init(const SideGraph* sg,
             const std::vector<std::string>* bases,
             const std::vector<NamedPath>* paths,
             bool forceUpperCase = false);

   /** Convert the graph into a new sidegraph, bases, and paths */
   void convert();

   /** Get the converted graph */
   const SideGraph* getOutGraph() const;

   /** Get the converted bases.  The ith element is the bases for the 
    * ith sequence in getOutGraph() */
   const std::vector<std::string>& getOutBases() const;

   /** Get the converted paths.  The ith element is the path in outgraph
    * that was converted from the ith path in the input bases */
   const std::vector<NamedPath>& getOutPaths() const;

   /** get dna from an input sequence */
   void getInDNA(const SGSegment& seg, std::string& outDNA) const;

   /** get dna from an output sequence */
   void getOutDNA(const SGSegment& seg, std::string& outDNA) const;   

   /** copied from halCommon.h -- dont want hal dep just for this*/
   static char reverseComplement(char c);
   static void reverseComplement(std::string& s);

protected:

   /** chop up a sequence from input graph by every join, and add fragments
    * as new sequences to the output graph, updating lookup structure */
   void convertSequence(const SGSequence* seq);
   void addOutSequence(const SGSequence* inSeq,
                       const SGPosition& first, const SGPosition& last);

   /** map a join onto the out graph by looking up its endpoints */
   void convertJoin(const SGJoin* join);

   /** make sure end points of join are ends of sequence */
   void verifyOutJoin(const SGJoin* join);

   /** map a path to the out graph */
   void convertPath(int inPathIdx);

   /** get all positions in range that are incident to one or more joins */
   int getIncidentJoins(const SGSide& start, const SGSide& end,
                        std::set<SGSide>& outSides) const;

   /** generate a name for a sequence in the output graph */
   std::string getOutSeqName(const SGSequence* inSeq, const SGPosition& first,
                             int length) const;
   
   const SideGraph* _inGraph;
   const std::vector<std::string>* _inBases;
   const std::vector<NamedPath>* _inPaths;
   
   SideGraph* _outGraph;
   std::vector<std::string> _outBases;
   std::vector<NamedPath> _outPaths;
   bool _forceUpper;

   // map Side Graph to Sequence Graph coords. 
   SGLookup _luTo;

   // order joins based on side 2 (SideGraph orders them on side 1)
   struct SGJoinPtrSide2Less {
      bool operator()(const SGJoin* j1, const SGJoin* j2) const;
   };
   typedef std::set<const SGJoin*, SGJoinPtrSide2Less> JoinSet2;
   JoinSet2 _joinSet2;
};

inline const SideGraph* Side2Seq::getOutGraph() const
{
  return _outGraph;
}

inline const std::vector<std::string>& Side2Seq::getOutBases() const
{
  return _outBases;
}

inline const std::vector<Side2Seq::NamedPath>& Side2Seq::getOutPaths() const
{
  return _outPaths;
}

inline void Side2Seq::getInDNA(const SGSegment& seg,
                               std::string& outDNA) const
{
  outDNA = _inBases->at(seg.getSide().getBase().getSeqID()).substr(
    seg.getMinPos().getPos(), seg.getLength());
  if (!seg.getSide().getForward())
  {
    reverseComplement(outDNA);
  }
  if (_forceUpper)
  {
    std::transform(outDNA.begin(), outDNA.end(), outDNA.begin(), ::toupper);
  }
}

inline void Side2Seq::getOutDNA(const SGSegment& seg,
                               std::string& outDNA) const
{
  outDNA = _outBases.at(seg.getSide().getBase().getSeqID()).substr(
    seg.getMinPos().getPos(), seg.getLength());
  if (!seg.getSide().getForward())
  {
    reverseComplement(outDNA);
  }
}

inline bool Side2Seq::SGJoinPtrSide2Less::operator()(const SGJoin* j1,
                                                     const SGJoin* j2) const
{
  return j1->getSide2() < j2->getSide2() || (
    j1->getSide2() == j2->getSide2() &&
    j1->getSide1() < j2->getSide1());
}



#endif
