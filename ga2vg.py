#!/usr/bin/env python

"""
Export to VG (in JSON format for now).
Modeled after ga2sam in ga4gh.cli
"""
import sys
import argparse

import ga4gh.client as client
import ga4gh.protocol as protocol
import ga4gh.converters as converters
import ga4gh.frontend as frontend

from ga4gh.cli import addClientGlobalOptions, getWorkarounds
from ga4gh.cli import addSequencesSearchParser, addJoinsSearchParser
from ga4gh.cli import addUrlArgument
from ga4gh.cli import RequestFactory

from sg2vg.vgconverter import VGConverter

def ga2vg_main(parser=None):
    #parse args
    if parser is None:
        parser = argparse.ArgumentParser(
            description="GA4GH VG conversion tool")
    #subparsers = parser.add_subparsers(title='subcommands',)
    addClientGlobalOptions(parser)
    addUrlArgument(parser)
    #addSequencesSearchParser(subparsers)
    #addJoinsSearchParser(subparsers)
    args = parser.parse_args()
    if "baseUrl" not in args:
        parser.print_help()
    else:
        ga2vg_run(args)

def ga2vg_run(args):
    # instantiate params
    searchSequencesRequest = RequestFactory(
        args).createSearchSequencesRequest()
    searchJoinsRequest = RequestFactory(
        args).createSearchJoinsRequest()
    workarounds = getWorkarounds(args)
    httpClient = client.HttpClient(
        args.baseUrl, args.verbose, workarounds, args.key)

    # do conversion
    vgConverter = VGConverter(
        httpClient, searchSequencesRequest, searchJoinsRequest, sys.stdout)
    vgConverter.convert()


    
if __name__ == "__main__":
    sys.exit(ga2vg_main())

     




