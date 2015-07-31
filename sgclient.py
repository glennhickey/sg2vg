#!/usr/bin/env python

"""
i can't get client api to work right now.  work around by reading json
directly, at least for time being
"""
import urllib2
import json
import sys
import os

def getJoins(url):
    """
    return all the joins in the graph at specified url in a list
    of json Joins
    """
    joins = []
    for i in xrange(sys.maxint):
        try:
            data = urllib2.urlopen("{}/joins/{}".format(url, i))
            join = json.loads(data)
            joins.append(join)
        except urllib2.HTTPError:
            break
    return joins


            
