/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#ifndef _DOWNLOAD_H
#define _DOWNLOAD_H

#include <string>
#include <vector>
#include <limits>
#include <sstream>

#include "SideGraph.h"


/** 
Keep all HTTP access code wrapped in this one class, as it's something
that's getting played aroudn with a lot. 
*/
class Download
{
public:
   
   Download();
   ~Download();

   static void init();
   static void cleanup();

   const char* postRequest(const std::string& url,
                           const std::vector<std::string>& headers,
                           const std::string& postData);

   struct MemoryStruct {
      char *memory;
      size_t size;
   };

protected:

   MemoryStruct _buffer;
};

#endif
