/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _JSON2SG_H
#define _JSON2SG_H

#include <vector>
#include <string>

#include "rapidjson/document.h"

#include "sidegraph.h"

/** put all JSON -> In-memory-sidegraph conversion in one place.  
We are not taking advantage of any schemas or anything, so expected
format is hard coded in each parse function.
*/
class JSON2SG
{
public:
   JSON2SG();
   ~JSON2SG();

   int parseJoins(const char* buffer, std::vector<SGJoin*>& outJoins);

   SGJoin* parseJoin(const rapidjson::Value& val);

   SGSide parseSide(const rapidjson::Value& val);

   SGPosition parsePosition(const rapidjson::Value& val);

protected:
   // hmm maybe i shouldnt be a class
};

#endif
