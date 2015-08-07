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

  for (int i = 0; i < outBases.size(); ++i)
  {
    CuAssertTrue(testCase, outBases[i].length() ==
                 outGraph->getSequence(i)->getLength());
    if (i == 3)
    {
      CuAssertTrue(testCase, outBases[i] == bases[1]);
    }
    else
    {
      CuAssertTrue(testCase, outBases[i] == string(outBases[i].length(), 'A'));
    }
  }

  // both paths should be broken up over the three segments
  CuAssertTrue(testCase, outPaths.size() == 2);
  CuAssertTrue(testCase, outPaths[0].second.size() == 3);
  CuAssertTrue(testCase, outPaths[0].second.size() == 3);

  CuAssertTrue(testCase, outPaths[0].second[0] ==
               SGSegment(SGSide(SGPosition(0, 0), true), 4));
  CuAssertTrue(testCase, outPaths[0].second[1] ==
               SGSegment(SGSide(SGPosition(1, 0), true), 1));
  CuAssertTrue(testCase, outPaths[0].second[2] ==
               SGSegment(SGSide(SGPosition(2, 0), true), 5));

  CuAssertTrue(testCase, outPaths[1].second[0] ==
               SGSegment(SGSide(SGPosition(0, 0), true), 4));
  CuAssertTrue(testCase, outPaths[1].second[1] ==
               SGSegment(SGSide(SGPosition(3, 0), true), 1));
  CuAssertTrue(testCase, outPaths[1].second[2] ==
               SGSegment(SGSide(SGPosition(2, 0), true), 5));

  CuAssertTrue(testCase, outPaths[0].first == paths[0].first);
  CuAssertTrue(testCase, outPaths[1].first == paths[1].first);
}


///////////////////////////////////////////////////////////
//  inversion test
// Make sure all 4 types of joins are converted
// 
///////////////////////////////////////////////////////////
void inversionTest(CuTest *testCase)
{
  CuAssertTrue(testCase, true);

  SideGraph sg;
  sg.addSequence(new SGSequence(0, 20, "seq1"));
  sg.addSequence(new SGSequence(1, 10, "seq2"));
  
  SGPosition p1a(0, 5);
  SGPosition p1b(0, 9);
  SGPosition p1c(0, 10);
  SGPosition p2a(1, 2);
  SGPosition p2b(1, 4);
  SGPosition p2c(1, 6);

  // R->F
  sg.addJoin(new SGJoin(SGSide(p1a, false), SGSide(p2a, true)));
  // R->R
  sg.addJoin(new SGJoin(SGSide(p2a, false), SGSide(p2b, false)));
  // F->R
  sg.addJoin(new SGJoin(SGSide(p2b, true), SGSide(p1c, false)));
  // F->F
  sg.addJoin(new SGJoin(SGSide(p1b, true), SGSide(p2c, true)));
  
  vector<string> bases(2);
  bases[0] = "ACCTGACCATAGGCATGGGC";
  bases[1] = "TCCGCCTAAA";
  
  vector<Side2Seq::NamedPath> paths(2);

  // follow joins "left to right"
  paths[0].first = "path1";
  paths[0].second.push_back(SGSegment(SGSide(SGPosition(0, 0), true), 6));
  paths[0].second.push_back(SGSegment(SGSide(p2a, true), 1));
  paths[0].second.push_back(SGSegment(SGSide(p2b, false), 1));
  paths[0].second.push_back(SGSegment(SGSide(p1b, false), 2));
  paths[0].second.push_back(SGSegment(SGSide(p2c, true), 2));

  // same path but in other direction
  paths[1].first = "path2";
  paths[1].second.push_back(SGSegment(SGSide(SGPosition(1, 9), false), 2));
  paths[1].second.push_back(SGSegment(SGSide(p1b, true), 2));
  paths[1].second.push_back(SGSegment(SGSide(p2b, true), 1));
  paths[1].second.push_back(SGSegment(SGSide(p2a, false), 1));
  paths[1].second.push_back(SGSegment(SGSide(p1a, false), 6));

  Side2Seq converter;
  converter.init(&sg, &bases, &paths);
  try
  {
    converter.convert();
  }
  catch(exception& e)
  {
    // inside convert, are some sanity checks making sure the paths
    // get converted properly. we'll be lazy and rely on this code
    // to make sure the paths got converted. 
    cerr << "Exception caught " << e.what() << endl;
    CuAssertTrue(testCase, false);
  }

  const SideGraph* outGraph = converter.getOutGraph();
  const vector<string> outBases = converter.getOutBases();
  const vector<Side2Seq::NamedPath> outPaths = converter.getOutPaths();

  // first sequence should be cut up into 4, and 2nd into 6
  CuAssertTrue(testCase, outGraph->getNumSequences() == 10);
  CuAssertTrue(testCase, outGraph->getSequence(0)->getLength() == 6);
  CuAssertTrue(testCase, outGraph->getSequence(1)->getLength() == 3);
  CuAssertTrue(testCase, outGraph->getSequence(2)->getLength() == 2);
  CuAssertTrue(testCase, outGraph->getSequence(3)->getLength() == 9);

  CuAssertTrue(testCase, outGraph->getSequence(4)->getLength() == 2);
  CuAssertTrue(testCase, outGraph->getSequence(5)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(6)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(7)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(8)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(9)->getLength() == 4);

  // joins

  // 4 original joins as mapped to new graph
  SGJoin j1(SGSide(SGPosition(0, 5), false),
            SGSide(SGPosition(5, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j1) != NULL);

  SGJoin j2(SGSide(SGPosition(5, 0), false),
            SGSide(SGPosition(7, 0), false));
  CuAssertTrue(testCase, outGraph->getJoin(&j2) != NULL);

  SGJoin j3(SGSide(SGPosition(7, 0), true),
            SGSide(SGPosition(2, 1), false));
  CuAssertTrue(testCase, outGraph->getJoin(&j3) != NULL);
  
  SGJoin j4(SGSide(SGPosition(2, 0), true),
            SGSide(SGPosition(9, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j3) != NULL);
  
  // 3 new joins added by fragmentation to sequence 0
  for (int i = 0; i < 3; ++i)
  {
    SGJoin j5(SGSide(SGPosition(i, outGraph->getSequence(i)->getLength() - 1),
                     false),
              SGSide(SGPosition(i + 1, 0), true));
    CuAssertTrue(testCase, outGraph->getJoin(&j5) != NULL);
  }

  // 5 new joins added by fragmentation to sequence 1
  for (int i = 5; i < 9; ++i)
  {
    SGJoin j5(SGSide(SGPosition(i, outGraph->getSequence(i)->getLength() - 1),
                     false),
              SGSide(SGPosition(i + 1, 0), true));
    CuAssertTrue(testCase, outGraph->getJoin(&j5) != NULL);
  }
  
  CuAssertTrue(testCase, outGraph->getJoinSet()->size() == 12);
}

///////////////////////////////////////////////////////////
//  doubleCut test
// case that first came up in camel-brca1 where segment cut
// failed because of equivalent join sides being processed
// when fragmenting segment 
// 
///////////////////////////////////////////////////////////
void doubleCutTest(CuTest *testCase)
{
  CuAssertTrue(testCase, true);

  SideGraph sg;
    
  SGPosition p1a(0, 5);
  SGPosition p1b(0, 6);
  SGPosition p1c(0, 10);
  
  sg.addSequence(new SGSequence(0, 20, "seq1"));
  sg.addJoin(new SGJoin(SGSide(p1a, false), SGSide(p1c, true)));
  sg.addJoin(new SGJoin(SGSide(p1b, true), SGSide(p1c, false)));

  vector<string> bases(1);
  bases[0] = "ACCTGACCATAGGCATGGGC";

  vector<Side2Seq::NamedPath> paths;

  Side2Seq converter;
  converter.init(&sg, &bases, &paths);
  try
  {
    converter.convert();
  }
  catch(exception& e)
  {
    cerr << "Exception caught " << e.what() << endl;
    CuAssertTrue(testCase, false);
  }

  const SideGraph* outGraph = converter.getOutGraph();
  CuAssertTrue(testCase, outGraph->getNumSequences() == 4);
  CuAssertTrue(testCase, outGraph->getSequence(0)->getLength() == 6);
  CuAssertTrue(testCase, outGraph->getSequence(1)->getLength() == 4);
  CuAssertTrue(testCase, outGraph->getSequence(2)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(3)->getLength() == 9);
}

///////////////////////////////////////////////////////////
//  rev2bSnpJoin test
// case that first came up in lrc_kir where sanity check
// fails on a seemingly straightforward 2base reverse snp
// 
///////////////////////////////////////////////////////////
void rev2bSnpJoinTest(CuTest *testCase)
{
  CuAssertTrue(testCase, true);

  SideGraph sg;
    
  SGPosition p1a(0, 5);
  SGPosition p2a(1, 0);
  
  sg.addSequence(new SGSequence(0, 20, "seq1"));
  sg.addSequence(new SGSequence(1, 2, "seq2"));
  sg.addJoin(new SGJoin(SGSide(p1a, true), SGSide(p2a, false)));

  vector<string> bases(2);
  bases[0] = "ACCTGACCATAGGCATGGGC";
  bases[1] = "TA";

  vector<Side2Seq::NamedPath> paths;

  Side2Seq converter;
  converter.init(&sg, &bases, &paths);
  try
  {
    converter.convert();
  }
  catch(exception& e)
  {
    cerr << "Exception caught " << e.what() << endl;
    CuAssertTrue(testCase, false);
  }

  const SideGraph* outGraph = converter.getOutGraph();

  CuAssertTrue(testCase, outGraph->getNumSequences() == 4);
  CuAssertTrue(testCase, outGraph->getSequence(0)->getLength() == 5);
  CuAssertTrue(testCase, outGraph->getSequence(1)->getLength() == 15);
  CuAssertTrue(testCase, outGraph->getSequence(2)->getLength() == 1);
  CuAssertTrue(testCase, outGraph->getSequence(3)->getLength() == 1);

  // original join
  SGJoin j1(SGSide(SGPosition(1, 0), true),
            SGSide(SGPosition(2, 0), false));
  CuAssertTrue(testCase, outGraph->getJoin(&j1) != NULL);

  // 1 new join added by fragmentation of sequence 0
  SGJoin j2(SGSide(SGPosition(0, 4), false),
            SGSide(SGPosition(1, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j2) != NULL);

  // 1 new join added by fragmentation of sequence 1
  SGJoin j3(SGSide(SGPosition(2, 0), false),
            SGSide(SGPosition(3, 0), true));
  CuAssertTrue(testCase, outGraph->getJoin(&j3) != NULL);
  
  CuAssertTrue(testCase, outGraph->getJoinSet()->size() == 3);

}


CuSuite* side2SeqTestSuite(void) 
{
  CuSuite* suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, simpleTest);
  SUITE_ADD_TEST(suite, inversionTest);
  SUITE_ADD_TEST(suite, doubleCutTest);
  SUITE_ADD_TEST(suite, rev2bSnpJoinTest);  
  return suite;
}
