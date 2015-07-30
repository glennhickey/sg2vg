#!/usr/bin/env python

"""
Export to VG (in JSON format for now).
Modeled after SAM converter in ga4gh.converters
"""

from ga4gh.converters import AbstractConverter

class JoinTable(object):
    """
    In memory table to look up JSON GA Joins by either their from
    or to sides, as well as by position on sequence.  
    """
    def __init(self):
        """ sorted list of (side, join) tuples """
        self._sideTable = []

    def addJoins(self, joins):
        """ add a JSON joins to sidetable, indexing by both sides """
        for join in joins:
            self._sideTable.append((join.side1, join))
            self._sideTable.append((join.side2, join))
        
        self._fromTable[_key(join.side1)] = join
        self._toTable[_key(join.side2)] = join

    def _key(side):
        """ make a tuple from a side for use as table key for now """
        return (side.base.sequenceID, side.base.position, side.forward)
    
        

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

    def convert(self):
        """
        iterate over all joins
        """
        joins = self._httpClient.searchJoins(self._searchJoinsRequest)
        for join in joins:
            print join
    
