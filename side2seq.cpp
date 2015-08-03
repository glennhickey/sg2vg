/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#include <iostream>
#include <sstream>

#include "side2seq.h"

using namespace std;


Side2Seq::Side2Seq() : _inGraph(NULL), _inBases(NULL), _inPaths(NULL),
                       _outGraph(NULL)
{
  
}

Side2Seq::~Side2Seq()
{
  reset();
}

void Side2Seq::reset()
{
  _inGraph = NULL;
  _inBases = NULL;
  _inPaths = NULL;
  delete _outGraph;
  _outBases.clear();
  _outPaths.clear();
}

void Side2Seq::init(const SideGraph* sg,
                    const vector<string>* bases,
                    const vector<NamedPath>* paths)
{
  reset();
  _inGraph = sg;
  _inBases = bases;
  _inPaths = paths;

  _outGraph = new SideGraph();
}

void Side2Seq::convert()
{

}
