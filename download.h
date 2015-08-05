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
#include <stdexcept>

#include "sidegraph.h"


/** 
Keep all HTTP access code wrapped in this one class, as it's something
that I'm messing around with too much. 
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

   const char* getRequest(const std::string& url,
                          const std::vector<std::string>& headers);                  
   struct MemoryStruct {
      char *memory;
      size_t size;
   };

   const char* getBuffer();
   void clearBuffer();

protected:

   MemoryStruct _buffer;
};

#endif
