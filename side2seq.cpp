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
  _joinSet2.clear();
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

  // re-index joins based on side2
  const SideGraph::JoinSet* js = _inGraph->getJoinSet();
  for (SideGraph::JoinSet::const_iterator i = js->begin(); i != js->end(); ++i)
  {
    _joinSet2.insert(*i);
  }
}

void Side2Seq::convert()
{
  // init lookup table with sequence names;
  vector<string> seqNames(_inGraph->getNumSequences());
  for (int i = 0; i < _inGraph->getNumSequences(); ++i)
  {
    seqNames[i] = _inGraph->getSequence(i)->getName();
  }
  _luTo.init(seqNames);

  // convert the sequences
  for (int i = 0; i < _inGraph->getNumSequences(); ++i)
  {
    convertSequence(_inGraph->getSequence(i));
  }

  // convert the joins
  const SideGraph::JoinSet* js = _inGraph->getJoinSet();
  for (SideGraph::JoinSet::const_iterator i = js->begin(); i != js->end(); ++i)
  {
    convertJoin(*i);
  }

  // convert the paths;
  _outPaths.resize(_inPaths->size());
  for (int i = 0; i < _inPaths->size(); ++i)
  {
    convertPath(i);
  }
}

void Side2Seq::convertSequence(const SGSequence* seq)
{
  // we exclude the very first and last sides as they don't induce breaks
  // (very first start woult have forward == true, for example..)
  SGSide start = SGSide(SGPosition(seq->getID(), 0), false);
  SGSide end = SGSide(SGPosition(seq->getID(), seq->getLength() - 1), true);
  set<SGSide> cutSides;
  if (seq->getLength() > 1)
  {
    getIncidentJoins(start, end, cutSides);
  }
  SGPosition first(seq->getID(), 0);
  for (set<SGSide>::iterator i = cutSides.begin(); i != cutSides.end(); ++i)
  {
    SGPosition last = i->getBase();
    assert(last.getSeqID() == first.getSeqID());
    if (i->getForward() == true)
    {
      // left side of base: don't include this position
      last.setPos(last.getPos() - 1);
    }
    // add it
    addOutSequence(seq, first, last);
    // add one because segments inclusive
    first.setPos(last.getPos() + 1);
  }

  // need to do one segment at end
  SGPosition last(seq->getID(), seq->getLength() - 1);
  addOutSequence(seq, first, last);
}

void Side2Seq::addOutSequence(const SGSequence* inSeq,
                              const SGPosition& first, const SGPosition& last)
{
  int length = last.getPos() - first.getPos() + 1; 
  assert(length > 0);
  const SGSequence* outSeq = _outGraph->addSequence(
    new SGSequence(inSeq->getID(), length,
                   getOutSeqName(inSeq, first, length)));
  _luTo.addInterval(first, SGPosition(outSeq->getID(), 0), length, false);

  // add the bases
  assert(_outBases.size() == outSeq->getID());
  _outBases.resize(outSeq->getID() + 1);
  getInDNA(inSeq->getID(), first.getPos(), length, _outBases.back());
}

void Side2Seq::convertJoin(const SGJoin* join)
{
  SGSide ret1 = _luTo.mapPosition(join->getSide1().getBase());
  assert(ret1.getBase() != SideGraph::NullPos);
  assert(ret1.getForward() == true);
  SGSide ret2 = _luTo.mapPosition(join->getSide2().getBase());
  assert(ret2.getBase() != SideGraph::NullPos);
  assert(ret2.getForward() == true);

  ret1.setForward(join->getSide1().getForward());
  ret2.setForward(join->getSide2().getForward());
  SGJoin* outJoin = new SGJoin(ret1, ret2);
  _outGraph->addJoin(outJoin);
}

void Side2Seq::convertPath(int inPathIdx)
{
  // outPaths gets resized somewhere above
  assert(_outPaths.size() > inPathIdx);

  const NamedPath& inPath = _inPaths->at(inPathIdx);
  NamedPath& outPath = _outPaths[inPathIdx];
  outPath.first = inPath.first;
  for (int i = 0; i < inPath.second.size(); ++i)
  {
    const SGSegment& seg = inPath.second[i];
    SGPosition firstPos = seg.getMinPos();
    SGPosition lastPos = seg.getMaxPos();
    if (seg.getSide().getForward() == false)
    {
      swap(firstPos, lastPos);
    }
    vector<SGSegment> frag;
    _luTo.getPath(firstPos, lastPos, frag);
    outPath.second.insert(outPath.second.end(), frag.begin(), frag.end());
  }
}

int Side2Seq::getIncidentJoins(const SGSide& start, const SGSide& end,
                               set<SGSide>& outSides) const
{
  outSides.clear();

  // joins indexed on side1;
  const SideGraph::JoinSet* joinSet1 = _inGraph->getJoinSet();
  SGJoin qj1(start, start);
  SideGraph::JoinSet::const_iterator i = joinSet1->lower_bound(&qj1);
  for (; i != joinSet1->end() && (*i)->getSide1() <= end; ++i)
  {
    outSides.insert((*i)->getSide1());
  }

  // joins indexed on side2;
  JoinSet2::const_iterator j = _joinSet2.lower_bound(&qj1);
  for (; j != _joinSet2.end() && (*j)->getSide2() <= end; ++j)
  {
    outSides.insert((*j)->getSide2());
  }
  
  return outSides.size();
}

string Side2Seq::getOutSeqName(const SGSequence* inSeq,
                               const SGPosition& first,
                               int length) const
{
  stringstream ss;
  ss << inSeq->getName() << "_" << first;
  return ss.str();
}
