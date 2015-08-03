/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.txt
 */

#include <string>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <fstream>
#include <getopt.h>

#include "sgclient.h"
#include "download.h"
#include "side2seq.h"

using namespace std;

void help(char** argv)
{
  cerr << "ga2vg: Convert GA4GH graph server to VG (JSON printed to stdout)\n"
       << "\nusage: " << argv[0] << " <URL> [options]\n"
       << "args:\n"
       << "    URL:  Input GA4GH graph server URL to convert\n"
       << "options:\n"
       << "    -h, --help         \n"
       << endl;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    help(argv);
    return 1;
  }

  optind = 2;
  while (true)
  {
    static struct option long_options[] =
       {
         {"help", no_argument, 0, 'h'},
       };
    int option_index = 0;
    int c = getopt_long(argc, argv, "h", long_options, &option_index);

    if (c == -1)
    {
      break;
    }
    
    switch(c)
    {
    case 'h':
    case '?':
      help(argv);
      exit(1);
    default:
      abort();
    }
  }

  Download::init();
  
  string url = argv[1];

  cout << "url " << url << endl;

  SGClient sgClient;
  sgClient.setURL(url);

  // ith element is bases for sequence with id i in side graph
  vector<string> bases;

  // ith element is <name, segment vector> for allele i
  vector<SGClient::NamedPath> paths;

  const SideGraph* sg = sgClient.downloadGraph(bases, paths);

  // convert side graph into sequence graph (which is stored 
  Side2Seq converter;
  converter.init(sg, &bases, &paths);
  converter.convert();

  cout << "INPUT " << endl;
  cout << *sgClient.getSideGraph() << endl;
  cout << "OUTPUT " << endl;
  cout << *converter.getOutGraph() << endl;

  
  
  Download::cleanup();
}
