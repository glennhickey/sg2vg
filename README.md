# sg2vg
Prototype code for converting [Global Alliance (Side Graph) Server](https://github.com/ga4gh/schemas/wiki/Human-Genome-Variation-Reference-(HGVR)-Pilot-Project#graph-format) graph to [VG](https://github.com/ekg/vg).

(c) 2015 Glenn Hickey. See [LICENSE](https://github.com/glennhickey/hal2sg/blob/development/LICENSE) for details.

See also:
* [vg2sg](https://github.com/glennhickey/vg2sg): Convert  [VG](https://github.com/ekg/vg) to  [Side Graph SQL](https://github.com/ga4gh/schemas/wiki/Human-Genome-Variation-Reference-(HGVR)-Pilot-Project#graph-format)
* [hal2sg](https://github.com/glennhickey/hal2sg): Convert  [HAL](https://github.com/glennhickey/hal) (output by [Cactus](https://github.com/glennhickey/progressiveCactus) and [CAMEL](https://github.com/adamnovak/sequence-graphs)) to [Side Graph SQL](https://github.com/ga4gh/schemas/wiki/Human-Genome-Variation-Reference-(HGVR)-Pilot-Project#graph-format)

## Algorithm

Download entire graph from server into memory.  Cut sequences so that all joins are incident to the first or last side of a sequence.  Print resulting graph in VG JSON format to stdout. 

## Instructions

**Dependency:** libcurl needs to be installed.

**Cloning:** Don't forget to clone submodules:

     git clone https://github.com/glennhickey/sg2vg.git --recursive

**Note** Start by verifying that the unit tests all pass:

	  make test

To run the converter:

	  sg2vg graph-url -u | vg view -J -v - > graph.vg

`graph-url` URL of graph server with version attached at end

`graph.vg` Output VG graph.  VG must be installed for `vg view` to work...

**Options**

    -h, --help
    -p, --pageSize     Number of records per POST request (default=1000).
    -u, --upper        Write all sequences in upper case. (RECOMMENDED)
    -a, --paths        Add a VG path for each input sequence.
    -n, --no-paths     Don't write any paths.     

