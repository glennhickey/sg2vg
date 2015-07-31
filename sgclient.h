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
   void setURL(const std::string& baseURL, const std::string& version);

   /** Download a whole Side Graph */
   const SideGraph* downloadGraph();

   /** Download joins into the Side Graph. returns number of joins */
   int downloadJoins(std::vector<const SGJoin*>& outJoins,
                     int idx = 0,
                     int numJoins = std::numeric_limits<int>::max(),
                     int referenceSetID = -1,
                     int variantSetID = -1);

   /** Download sequences into the Side Graph. returns number of joins */
   int downloadSequences(std::vector<const SGSequence*>& outSequences,
                         int idx = 0,
                         int numJoins = std::numeric_limits<int>::max());

   /** Download DNA for a sequence. Unlike other functions, the DNA
    * downloaded does not get stored in this object */
   int downloadDNA(sg_int_t sequenceID, std::string& outDNA);

   /** Download paths */
   // todo

   /** Get access to Side Graph that's been downloaded so far */
   const SideGraph* getSideGraph() const;
   
protected:

   static const std::string CTHeader;
   
   SideGraph* _sg;
   std::string _url;
   Download _download;

};


#endif
