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
#include <cstdio>
#include <getopt.h>

#include "sgclient.h"
#include "download.h"
#include "side2seq.h"
#include "sg2vgjson.h"

using namespace std;

void help(char** argv)
{
  cerr << "ga2vg: Convert GA4GH graph server to VG (JSON printed to stdout)\n"
       << "\nusage: " << argv[0] << " <URL> [options]\n"
       << "args:\n"
       << "    URL:  Input GA4GH graph server URL to convert\n"
       << "options:\n"
       << "    -h, --help         \n"
       << "    -p, --pageSize     Number of records per POST request "
       << "(default=" << SGClient::DefaultPageSize << ").\n"
       << "    -u, --upper        Write all sequences in upper case.\n"
       << "    -a, --paths        Add a VG path for each input sequence.\n"
       << endl;
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    help(argv);
    return 1;
  }

  int pageSize = SGClient::DefaultPageSize;
  bool upperCase = false;
  bool seqPaths = false;
  optind = 2;
  while (true)
  {
    static struct option long_options[] =
       {
         {"help", no_argument, 0, 'h'},
         {"page", required_argument, 0, 'p'},
         {"upper", no_argument, 0, 'u'},
         {"paths", no_argument, 0, 'a'}
       };
    int option_index = 0;
    int c = getopt_long(argc, argv, "hp:ua", long_options, &option_index);

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
    case 'p':
      pageSize = atoi(optarg);
      break;
    case 'u':
      upperCase = true;
      break;
    case 'a':
      seqPaths = true;
      break;
    default:
      abort();
    }
  }

  Download::init();
  
  string url = argv[1];

  SGClient sgClient;
  sgClient.setURL(url);
  sgClient.setOS(&cerr);
  sgClient.setPageSize(pageSize);

  // ith element is bases for sequence with id i in side graph
  vector<string> bases;

  // ith element is <name, segment vector> for allele i
  vector<SGClient::NamedPath> paths;

  const SideGraph* sg = sgClient.downloadGraph(bases, paths);

  // convert side graph into sequence graph (which is stored
  cerr << "Converting Side Graph to VG Sequence Graph" << endl;
  Side2Seq converter;
  converter.init(sg, &bases, &paths, upperCase, seqPaths, "&SG_");
  converter.convert();

  const SideGraph* outGraph = converter.getOutGraph();
  const vector<string>& outBases = converter.getOutBases();
  const vector<SGClient::NamedPath>& outPaths = converter.getOutPaths();
  
  // write to vg json
  cerr << "Writing VG JSON to stdout" << endl;
  SG2VGJSON jsonWriter;
  jsonWriter.init(&cout);
  jsonWriter.writeGraph(outGraph, outBases, outPaths);

  /*
  cerr << "INPUT " << endl;
  cerr << *sgClient.getSideGraph() << endl;
  cerr << "OUTPUT " << endl;
  cerr << *outGraph << endl;

  for (int i = 0; i < outPaths.size(); ++i)
  {
    cerr << "path " << i << ":\n";
    cerr << "input " << "name=" << outPaths[i].first;
    for (int j = 0; j < outPaths[i].second.size(); ++j)
    {
      cerr << outPaths[i].second[j] << ", ";
    }
    cerr << endl;
    cerr << "output " << "name=" << outPaths[i].first;
    for (int j = 0; j < outPaths[i].second.size(); ++j)
    {
      cerr << outPaths[i].second[j] << ", ";
    }
    cerr << endl;
  }
  */
  
  Download::cleanup();
}
