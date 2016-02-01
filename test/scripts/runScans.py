#!/usr/bin/env python

import os
import datetime
import re
import sys
import time
import traceback

from Configuration import ConfigFromFile
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from ConfigCase import ConfigCase


class RunScans(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)


    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        parser.add_argument("-m","--symbolMap",help="symbolMap file to use. This assumes that the launchers have been started.")
        parser.add_argument("configs",nargs='+',help="path to the config(s) to run")
        parser.add_argument("-n","--nbMeasurements",default=10,type=int,help="number of measurements to take for each fragment size [default: %(default)s]")
        parser.add_argument("-s","--sizes",default=(2048,),nargs="+",type=int,help="fragment sizes (in Bytes) to scan [default: %(default)s]")
        parser.add_argument("-r","--rms",default=0,type=float,help="relative rms of fragment size [default: %(default)s]")
        parser.add_argument("-q","--quick",action='store_true',help="run a quick scan with less sleep time")
        parser.add_argument("--short",action='store_true',help="run a short scan")
        parser.add_argument("--full",action='store_true',help="run the full scan")
        parser.add_argument("--fixPorts",action='store_true',help="fix the port numbers on FEROLs and RUs")
        parser.add_argument("-a","--append",action='store_true',help="append measurements to data file")
        parser.add_argument("-o","--outputDir",default=self._evbTesterHome+'/log/',help="output directory [default: %(default)s]")


    def doIt(self):
        if len(self.args['configs']) == 0:
            return False
        try:
            os.mkdir(self.args['outputDir'])
        except OSError:
            pass
        for configFile in self.args['configs']:
            configName = os.path.splitext(os.path.basename(configFile))[0]
            if os.path.isdir(configFile):
                configFile += "/"+configName+".xml"
            logFile = open(self.args['outputDir']+"/"+configName+".txt",'w')
            if self.args['verbose']:
                stdout = Tee(sys.stdout,logFile)
            else:
                startTime = time.strftime("%H:%M:%S", time.localtime())
                sys.stdout.write("%-32s: %s " % (configName,startTime))
                sys.stdout.flush()
                stdout = logFile

            success = self.runConfig(configName,configFile,stdout)

            if not self.args['verbose']:
                stopTime = time.strftime("%H:%M:%S", time.localtime())
                print(stopTime+" "+success)
        return True


    def runConfig(self,configName,configFile,stdout):
        try:
            if self.args['full']:
                sizes = [256, 512, 768, 1024, 1149, 1280, 1408, 1536, 1664, 1856, 2048, 2560,
                         3072, 4096, 6144, 8192, 10240, 12288, 16384]
            elif self.args['short']:
                sizes = [256, 512, 768, 1024, 1536, 2048, 2560, 4096, 8192, 12288, 16384]
            else:
                sizes = self.args['sizes']

            if self.args['symbolMap']:
                self._symbolMap = SymbolMap(self.args['symbolMap'])
            else:
                configDir = os.path.dirname(configFile)
                self._symbolMap = SymbolMap(configDir+"/symbolMap.txt")
                self.startLaunchers()
                time.sleep(1)

            outputFileName = self.args['outputDir']+"/"+configName+".dat"
            if self.args['append']:
                mode = 'a'
            else:
                mode = 'w'
                if os.path.exists(outputFileName):
                    t = os.path.getmtime(outputFileName)
                    dateStr = datetime.datetime.fromtimestamp(t).strftime("%Y%m%d_%H%M%S")
                    bakDir = self.args['outputDir']+'/'+dateStr
                    os.makedirs(bakDir)
                    os.rename(outputFileName,bakDir+"/"+configName+".dat")
            with open(outputFileName,mode) as dataFile:
                config = ConfigFromFile(self._symbolMap,configFile,self.args['fixPorts'],self.args['numa'])
                configCase = ConfigCase(config,stdout)
                configCase.prepare(configName)
                for fragSize in sizes:
                    fragSizeRMS = int(fragSize * self.args['rms'])
                    data = {}
                    data['fragSize'] = fragSize
                    data['fragSizeRMS'] = fragSizeRMS
                    data['measurement'] = configCase.runScan(fragSize,fragSizeRMS,self.args)
                    dataFile.write(str(data)+"\n")
                    dataFile.flush()
                del(configCase)
            returnValue = "\033[1;37;42m DONE \033[0m"
        except Exception as e:
            traceback.print_exc(file=stdout)
            returnValue = "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        finally:
            if not self.args['symbolMap']:
                self.stopLaunchers()
        return returnValue


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    runScans = RunScans()
    runScans.addOptions(parser)
    try:
        runScans.run( parser.parse_args() )
    except BadConfig:
        parser.print_help()
