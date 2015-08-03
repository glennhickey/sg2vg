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

#include <sstream>
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
             const std::vector<NamedPath>* paths);

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
   
   
protected:

   const SideGraph* _inGraph;
   const std::vector<std::string>* _inBases;
   const std::vector<NamedPath>* _inPaths;
   
   SideGraph* _outGraph;
   std::vector<std::string> _outBases;
   std::vector<NamedPath> _outPaths;
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



#endif
