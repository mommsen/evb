#!/usr/bin/env python

import argparse
import datetime
import os
import subprocess
import sys
import time

import messengers
from SymbolMap import SymbolMap
from TestCase import TestCase


class BadConfig(Exception):
    pass


class Tee(object):
    def __init__(self, *files):
        self._files = files

    def write(self, obj):
        for f in self._files:
            f.write(obj)

    def flush(self):
         for f in self._files:
            f.flush()


class TestRunner:

    def __init__(self):
        try:
            self._evbTesterHome = os.environ["EVB_TESTER_HOME"]
        except KeyError:
            print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
            sys.exit(2)

        # the launcher script to be executed on the remote host
        # check whether it exists (assuming a cluster wide file system)
        # or not before attempting to launch it on the remote hosts
        self._launcherScript = self._evbTesterHome + "/scripts/xdaqLauncher.py"
        if not os.path.exists(self._launcherScript):
            print("Could not find xdaq launcher script " + self._launcherScript + ".")
            print("Does the environment variable EVB_TESTER_HOME point to the correct directory ?")
            sys.exit(2)

        self.defaultFedSize = 2048.


    def addOptions(self,parser):
        parser.add_argument("-v","--verbose",action='store_true',help="print log info also to stdout")
        parser.add_argument("--numa",action='store_false',help="do not use NUMA settings")
        parser.add_argument("-l","--launchers",choices=('start','stop'),help="start/stop xdaqLaunchers")
        parser.add_argument("--logDir",default=self._evbTesterHome+'/log/',help="log directory [default: %(default)s]")
        parser.add_argument("--dummyXdaq", action = 'store_true', default = False, help="run dummyXdaq.py instead of xdaq.exe (useful for development)")
        parser.add_argument("--logLevel", type = str, default = None, help="set XDAQ log level")


    def addScanOptions(self,parser):
        parser.add_argument("-s","--sizes",default=(self.defaultFedSize,),nargs="+",type=int,help="fragment sizes (in Bytes) to scan [default: %(default)s]")
        parser.add_argument("--short",action='store_true',help="run a short scan")
        parser.add_argument("--full",action='store_true',help="run the full scan")
        parser.add_argument("--compl",action='store_true',help="run the scan points not included in short")
        parser.add_argument("-r","--relsizes",nargs="*",type=float,help="relative event sizes to scan")
        parser.add_argument("--rms",default=0,type=float,help="relative rms of fragment size [default: %(default)s]")
        parser.add_argument("-n","--nbMeasurements",default=20,type=int,help="number of measurements to take for each fragment size [default: %(default)s]")
        parser.add_argument("-q","--quick",action='store_true',help="run a quick scan with less sleep time")
        parser.add_argument("--long",action='store_true',help="additional sleep time, mostly useful for running with F3")
        parser.add_argument("-a","--append",action='store_true',help="append measurements to data file")
        parser.add_argument("-o","--outputDir",default=self._evbTesterHome+'/log/',help="output directory [default: %(default)s]")


    def getFedSizes(self):
        allSizes = [256,512,768,1024,1149,1280,1408,1536,1664,1856,2048,2560,4096,8192,12288,16384]
        shortSizes = [256,512,768,1024,1280,1536,2048,2560,4096,8192,12288,16384]
        if self.args['relsizes']:
            return [ int(r * self.defaultFedSize) for r in self.args['relsizes'] ]
        elif self.args['full']:
            return allSizes
        elif self.args['short']:
            return shortSizes
        elif self.args['compl']:
            return [ s for s in allSizes if s not in shortSizes ]
        else:
            # explicit list of fragment sizes specified on the command line
            return self.args['sizes']


    def scanFedSizes(self,testName,test):
        outputFileName = self.args['outputDir']+"/"+testName+".dat"
        if self.args['append']:
            mode = 'a'
        else:
            mode = 'w'
            if os.path.exists(outputFileName):
                t = os.path.getmtime(outputFileName)
                dateStr = datetime.datetime.fromtimestamp(t).strftime("%Y%m%d_%H%M%S")
                bakDir = self.args['outputDir']+'/'+dateStr
                os.makedirs(bakDir)
                os.rename(outputFileName,bakDir+'/'+os.path.basename(outputFileName))

        with open(outputFileName,mode) as dataFile:
            for fragSize in self.getFedSizes():
                fragSizeRMS = int(fragSize * self.args['rms'])
                data = {}
                data['fragSize'] = fragSize
                data['fragSizeRMS'] = fragSizeRMS
                data['measurement'] = test.runScan(fragSize,fragSizeRMS,self.args)
                dataFile.write(str(data)+"\n")
                dataFile.flush()


    def run(self,args):
        self.args = vars(args)
        #print(self.args)
        #return
        try:
            os.mkdir(self.args['logDir'])
        except OSError:
            pass
        try:
            self.doIt()
        except KeyboardInterrupt:
            pass


    def startLaunchers(self):

        launcherCmd = "cd /tmp && rm -f /tmp/core.* && export XDAQ_ROOT="+os.environ["XDAQ_ROOT"]+" && "+ self._launcherScript + " "
        if not self.args['verbose']:
            launcherCmd += "--logDir "+self.args['logDir']+" "
        if self.args['numa']:
            launcherCmd += "--useNuma "

        if self.args['dummyXdaq']:
            launcherCmd += "--dummyXdaq "

        for launcher in self._symbolMap.launchers:
            if self.args['verbose']:
                print("Starting launcher on "+launcher[0]+":"+str(launcher[1]))
            subprocess.Popen(["ssh",

                              # avoid ssh waiting for confirmation of unknown host key
                              # but keep keys learned this way in a separate file
                              # similar to what wassh does by default
                              "-o","UserKnownHostsFile=" + os.path.expanduser("~/.evbtest_known_hosts"),
                              "-o","StrictHostKeyChecking=no",

                              "-x","-n",launcher[0],launcherCmd+str(launcher[1])])


    def stopLaunchers(self):
        for launcher in self._symbolMap.launchers:
            if self.args['verbose']:
                print("Stopping launcher on "+launcher[0]+":"+str(launcher[1]))

            import xmlrpclib
            server = xmlrpclib.ServerProxy("http://%s:%s" % (launcher[0],launcher[1]))
            server.stopLauncher()
