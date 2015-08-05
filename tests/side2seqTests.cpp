/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.txt
 */
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <sstream>
#include "unitTests.h"
#include "sidegraph.h"
#include "side2seq.h"

using namespace std;


///////////////////////////////////////////////////////////
//  Make sure trivial graph runs through ok
//  
///////////////////////////////////////////////////////////
void simpleTest(CuTest *testCase)
{
  CuAssertTrue(testCase, true);

  // build easy side graph with one snp. 

  SideGraph sg;
  sg.addSequence(new SGSequence(0, 10, "seq1"));
  sg.addSequence(new SGSequence(1, 1, "snp"));
  sg.addJoin(new SGJoin(SGSide(SGPosition(0, 3), false),
                        SGSide(SGPosition(1, 0), true)));
  sg.addJoin(new SGJoin(SGSide(SGPosition(1, 0), false),
                        SGSide(SGPosition(0, 5), true)));
  vector<string> bases(2);
  bases[0] = string(10, 'A');
  bases[1] = string(1, 'G');
  
  vector<Side2Seq::NamedPath> paths(2);
  paths[0].first = "path1";
  paths[0].second.push_back(SGSegment(SGSide(SGPosition(0, 0), true), 10));

  paths[1].first = "path2";
  paths[1].second.push_back(SGSegment(SGSide(SGPosition(0, 0), true), 4));
  paths[1].second.push_back(SGSegment(SGSide(SGPosition(1, 0), true), 1));
  paths[1].second.push_back(SGSegment(SGSide(SGPosition(0, 5), true), 5));

  Side2Seq converter;
  converter.init(&sg, &bases, &paths);
  converter.convert();

  const SideGraph* outGraph = converter.getOutGraph();
  const vector<string> outBases = converter.getOutBases();
  const vector<Side2Seq::NamedPath> outPaths = converter.getOutPaths();

  // expect sequences of length 4, 1, 5, 1
  CuAssertTrue(testCase, outGraph->getNumSequences() == 4);
  CuAssertTrue(testCase, outGraph->getSequence(0)->getLength() == 4);
  CuAssertTrue(testCase, outGraph->getSequence(1)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(2)->getLength() == 5);
  CuAssertTrue(testCase, outGraph->getSequence(3)->getLength() == 1);

  // expect 2 extra joins
  CuAssertTrue(testCase, outGraph->getJoinSet()->size() == 4);
  SGJoin j1(SGSide(SGPosition(0, 3), false),
            SGSide(SGPosition(1, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j1) != NULL);
  SGJoin j2(SGSide(SGPosition(0, 3), false),
            SGSide(SGPosition(3, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j2) != NULL);
  SGJoin j3(SGSide(SGPosition(1, 0), false),
            SGSide(SGPosition(2, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j3) != NULL);
  SGJoin j4(SGSide(SGPosition(3, 0), false),
            SGSide(SGPosition(2, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j4) != NULL);
}

CuSuite* side2SeqTestSuite(void) 
{
  CuSuite* suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, simpleTest);
  return suite;
}
