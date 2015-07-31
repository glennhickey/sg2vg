#!/usr/bin/env python

"""
Export to VG (in JSON format for now).
Modeled after SAM converter in ga4gh.converters
"""

import bisect

from ga4gh.converters import AbstractConverter
from sg2vg.sgclient import getJoins

class JoinTable(object):
    """
    In memory table to look up JSON GA Joins that touch a
    particular subsequence.  
    """
    def __init(self):
        """ sorted list of (side, join) tuples """
        self._sideTable = []
        self._keyList = []

    def addJoins(self, joins):
        """ add a JSON joins to sidetable, indexing by both sides """
        for join in joins:
            self._sideTable.append((join.side1, join))
            self._sideTable.append((join.side2, join))

        # keep list of joins in sorted order by both sides
        # this way we can find all joins that touch an given segment
        # (either as from or to)
        self._sideTable.sort(self._sideTable, key=JoinTable._key)
        
        # need sorted keys to use bisect module
        self._keyList = [_key(x) for x in _sideTable]
        

    def _key((side, join)):
        """ make a tuple from a side for use as table key for now """
        return (side.base.sequenceID, side.base.position)

    def joinsInSegment(self, sequenceID, leftPos, rightPos):
        """
        left and right pos are offsets from beginning of sequence
        find all joins that touch this segment.  a join can be
        returned twice (for now)
        """
        left = bisect_left(_keyList, (sequenceID, leftPos))
        right = bisect_right(_keyList, (sequenceID, rightPos))

        # need to verify off-by-1 here?
        for i in xrange(left, right + 1):
            yield _sideTable[i][1]
                        

class VGConverter(AbstractConverter):
    """
    Convert sequences and joins requests into VG (JSON)
    (ignoring paths for now)
    """
    def __init__(self, httpClient, searchSequencesRequest,
                 searchJoinsRequest, outputStream):
        super(VGConverter, self).__init__(httpClient)
        self._searchSequencesRequest = searchSequencesRequest
        self._searchJoinsRequest = searchJoinsRequest
        self._outputStream = outputStream
        self._jt = JoinTable()
        self._sequences = None

    def convert(self):
        """
        iterate over all joins
        """
        joins = self._downloadJoins()

        for join in joins:
            print join
        
        # index joins by both sides
        self._jt.addJoins(joins)

        # read every sequence
        self._sequences = self._downloadSequnces()

        self._convertSequences()
        self._convertJoins()

    def _downloadJoins(self):
        # disable api, replace with direct query
        #joinsGen = self._httpClient.searchJoins(self._searchJoinsRequest)
        #joins = [join for join in joinsGen]
        joins = getJoins(self._httpClient._urlPrefix)
        return joins

    def _downloadSequences(self):
        # todo (once joins are working)
        return []

    def _convertSequences(self):
        """
        Convert every side graph sequence into a list of VG Nodes by
        scanning left-to-write and cutting the sequence everytime a join
        is encountered.
        """ 
 
    
        
        
