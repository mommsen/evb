#!/usr/bin/env python

import collections
import datetime
import gzip
import os
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import XMLtools


class CheckFBthroughput:

    def __init__(self,args):
        self.args = vars(args)
        self.config = self.readConfigFile(self.args['xmlConfigFile'])
        self.params = self.readParamFile(self.args['paramFile'])


    def readConfigFile(self,configFile):
        if os.path.exists(configFile):
            ETroot = ET.parse(configFile).getroot()
        elif os.path.exists(configFile+".gz"):
            f = gzip.open(configFile+".gz") #gzip.open() doesn't support the context manager protocol needed for it to be used in a 'with' statement.
            ETroot = ET.parse(f).getroot()
            f.close()
        else:
            raise IOError(configFile+"(.gz) does not exist")
        return ETroot


    def readParamFile(self,paramFile):
        params = {}
        with open(paramFile) as file:
            try:
                for line in file:
                    try:
                        (fedIds,a,b,c,rms) = line.split(',',5)
                    except ValueError:
                        (fedIds,a,b,c,rms,_) = line.split(',',6)
                    try:
                        params[fedIds] = (float(a),float(b),float(c))
                    except ValueError:
                        pass
            except Exception as e:
                print("Failed to parse: "+line)
                raise e
        return params


    def getRUs(self):
        rus = []
        xcns = re.match(r'\{(.*?)\}Partition',self.config.tag).group(1) ## Extract xdaq namespace
        for context in self.config.getiterator(str(QN(xcns,'Context'))):
            for app in context.getiterator(str(QN(xcns,'Application'))):
                if app.attrib['class'] in ('evb::EVM','evb::RU'):
                    fedIds = []
                    group = app.attrib['group'].split(',')
                    for child in app:
                        if 'properties' in child.tag:
                            for prop in child:
                                if 'fedSourceIds' in prop.tag:
                                    for item in prop:
                                        fedIds.append(item.text)
                    rus.append((group,fedIds))
        return rus


    def calculateThroughput(self):
        eventSize = 0
        tooLargeFB = []
        for ru in self.getRUs():
            superFragmentSize = 0.
            for fed in ru[1]:
                try:
                    param = self.params[fed]
                    fedSize = param[0] + param[1]*self.args['relEventSize'] + param[2]*self.args['relEventSize']**2
                except KeyError:
                    fedSize = 2048
                    print("WARNING: FED %s not found in paramFile. Using %d Bytes" % (fed,fedSize))
                superFragmentSize += int(fedSize+4)&~0x7
            eventSize += superFragmentSize/1e6
            throughput = (superFragmentSize*self.args['triggerRate'])/1e6
            if throughput > self.args['maxThroughput']:
                tooLargeFB.append((ru[0][1:],throughput,ru[1]))
        print("Total event size: %1.2f MB"%(eventSize))
        for fb in tooLargeFB:
            print("  %1.2f GB/s from %d FEDs : %s %s" % (fb[1],len(fb[2]),fb[0],fb[2]))


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("xmlConfigFile",help="configuration file")
    parser.add_argument("paramFile",help="calculate the FED fragment sizes using the parameters in the given file")
    parser.add_argument("relEventSize",type=float,help="scale the event size by this factor")
    parser.add_argument("-t","--triggerRate",default=100,type=int,help="L1 trigger rate in kHz [default: %(default)s]")
    parser.add_argument("-m","--maxThroughput",default=3.3,type=float,help="report FB which have a throughput exceeding this value in GB/s [default: %(default)s]")
    checkFBthroughput = CheckFBthroughput( parser.parse_args() )
    checkFBthroughput.calculateThroughput()
