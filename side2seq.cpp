/*
 * Copyright (C) 2015 by Glenn Hickey (hickey@soe.ucsc.edu)
 *
 * Released under the MIT license, see LICENSE.cactus
 */

#include <iostream>
#include <sstream>
#include <algorithm>

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
  int firstIdx = _outGraph->getNumSequences();
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

  // chain all the added seqeunces with new joins
  for (int j = firstIdx + 1; j < _outGraph->getNumSequences(); ++j)
  {
    const SGSequence* fs = _outGraph->getSequence(j-1);
    const SGSequence* ts = _outGraph->getSequence(j);
    SGSide side1(SGPosition(fs->getID(), fs->getLength() - 1), false);
    SGSide side2(SGPosition(ts->getID(), 0), true);
    const SGJoin* newJoin = _outGraph->addJoin(new SGJoin(side1, side2));
    verifyOutJoin(newJoin);
  }
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
  getInDNA(SGSegment(SGSide(first, true), length), _outBases.back());
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
  verifyOutJoin(outJoin);
  _outGraph->addJoin(outJoin);
}

void Side2Seq::verifyOutJoin(const SGJoin* join)
{
  const SGPosition& p1 = join->getSide1().getBase();
  const SGSequence* s1 = _outGraph->getSequence(p1.getSeqID());
  const SGPosition& p2 = join->getSide2().getBase();
  const SGSequence* s2 = _outGraph->getSequence(p2.getSeqID());

  bool g1 = (p1.getPos() == 0 && join->getSide1().getForward()) ||
     (p1.getPos() == s1->getLength() - 1 && !join->getSide1().getForward());
  bool g2 = (p2.getPos() == 0 && join->getSide2().getForward()) ||
     (p2.getPos() == s2->getLength() - 1 && !join->getSide2().getForward());

  if (!g1 || !g2)
  {
    stringstream ss;
    ss << "Output join, " << *join << " is bad because it doesn't abut"
       << " sequence ends"
       << ". This is probably a BUG, please report it!";
    throw runtime_error(ss.str());
  }
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
    
    // to verify bases match
    string seq1;
    getInDNA(seg, seq1);

    string seq2;
    string buf;
    for (int j = 0; j < frag.size(); ++j)
    {
      assert(frag[j].getSide().getBase().getSeqID() <
             _outGraph->getNumSequences());
      
      getOutDNA(frag[j], buf);
      seq2 += buf;
      if (j > 0)
      {
        SGJoin bridge(frag[j-1].getOutSide(),
                      frag[j].getInSide());
        if (_outGraph->getJoin(&bridge) == NULL)
        {
          stringstream ss;
          ss << "Error converting " << inPathIdx << "th path with name="
             << inPath.first << ": missing join " << bridge
             << ". This is probably a BUG, please report it!";
          throw runtime_error(ss.str());
        }
      }
      outPath.second.push_back(frag[j]);
    }
    transform(seq1.begin(), seq1.end(), seq1.begin(), ::toupper);
    transform(seq2.begin(), seq2.end(), seq2.begin(), ::toupper);
    if (seq1 != seq1)
    {
      stringstream ss;
      ss << "Error converting " << inPathIdx << "th path with name="
         << inPath.first << ": output path does not match"
         << ". This is probably a BUG, please report it!";
      throw runtime_error(ss.str());
    }
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

char Side2Seq::reverseComplement(char c)
{
  switch (c)
  {
  case 'A' : return 'T'; 
  case 'a' : return 't'; 
  case 'C' : return 'G'; 
  case 'c' : return 'g';
  case 'G' : return 'C';
  case 'g' : return 'c';
  case 'T' : return 'A';
  case 't' : return 'a';
  default : break;
  }
  return c;
}

void Side2Seq::reverseComplement(std::string& s)
{
  if (!s.empty())
  {
    size_t j = s.length() - 1;
    size_t i = 0;
    char buf;
    do
    {
      while (j > 0 && s[j] == '-')
      {
        --j;
      }
      while (i < s.length() - 1 && s[i] == '-')
      {
        ++i;
      }
      
      if (i >= j || s[i] == '-' || s[j] == '-')
      {
        if (i == j && s[i] != '-')
        {
          s[i] = reverseComplement(s[i]);
        }
        break;
      }

      buf = reverseComplement(s[i]);
      s[i] = reverseComplement(s[j]);
      s[j] = buf;

      ++i;
      --j;
    } while (true);
  }
}
