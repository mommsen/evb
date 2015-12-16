#!/usr/bin/env python

import os
import re


def getConfig(string):
    """Extract number of streams, readout units, builder units, and number of streams
    per frl from strings such as 8x1x2 or 16s8fx2x4 (i.e 8,1,2,0 in the first case,
    16,2,4,2 in the second)
    """
    config_regex = re.compile("## (\w*) configuration with (\w*)/(\w*)$")
    r = config_regex.match(string)
    nstreams = None
    strperfrl = None
    nrus = None
    nbus = None
    if r is not None:
        config,builder,protocol = r.groups()

        case = config.split('x')

        if len(case) == 2:
            nrus = int(case[0])
            nbus = int(case[1])
            nstreams = 8*(nrus-1)+1
        elif len(case) > 2:
            strperfrl = 1
            pattern = re.compile(r'([0-9]+)s([0-9]+)f')
            match = pattern.match(case[0])
            if match:
                nstreams = int(match.group(1))
                if nstreams > int(match.group(2)):
                    strperfrl = 2
            else:
                nstreams = int(case[0])
            nrus = int(case[1])-1
            nbus = int(case[2])

    return nstreams,nrus,nbus,strperfrl


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("input",help="Path to old-style server.csv")
    parser.add_argument("output",help="Name of new-style dat file")
    args = vars( parser.parse_args() )
    nstreams = None
    entry = {}
    with open(args['input'],'r') as input:
        while nstreams is None:
            line = input.readline()
            nstreams,nrus,nbus,strperfrl = getConfig(line)
        with open(args['output'],'w') as output:
            for line in input:
                try:
                    sizes,rates = line.split(':')
                    superFragmentSize = int(sizes.split(',')[1])
                    entry['fragSize'] = superFragmentSize / ((nstreams-1) / nrus)
                    entry['fragSizeRMS'] = 0
                    entry['measurement'] = []
                    for rate in rates.split(','):
                        r = {}
                        s = {}
                        r['EVM0'] = int(rate)
                        s['EVM0'] = 1024
                        for n in range(nrus):
                            r['RU'+str(n+1)] = int(rate)
                            s['RU'+str(n+1)] = superFragmentSize
                        for n in range(nbus):
                            r['BU'+str(n)] = int(rate) / nbus
                            s['BU'+str(n)] = superFragmentSize * nrus + 1024
                        entry['measurement'].append({'rates':r,'sizes':s})

                    output.write(str(entry)+'\n')
                except ValueError:
                    pass
