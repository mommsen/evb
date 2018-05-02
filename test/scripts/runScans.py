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
        parser.add_argument("-n","--nbMeasurements",default=20,type=int,help="number of measurements to take for each fragment size [default: %(default)s]")
        parser.add_argument("-r","--relsizes",nargs="*",type=float,help="relative event sizes to scan")
        parser.add_argument("-s","--sizes",default=(ConfigCase.defaultFedSize,),nargs="+",type=int,help="fragment sizes (in Bytes) to scan [default: %(default)s]")
        parser.add_argument("--rms",default=0,type=float,help="relative rms of fragment size [default: %(default)s]")
        parser.add_argument("-q","--quick",action='store_true',help="run a quick scan with less sleep time")
        parser.add_argument("--long",action='store_true',help="additional sleep time, mostly useful for running with F3")
        parser.add_argument("--short",action='store_true',help="run a short scan")
        parser.add_argument("--full",action='store_true',help="run the full scan")
        parser.add_argument("--compl",action='store_true',help="run the scan points not included in short")
        parser.add_argument("--fixPorts",action='store_true',help="fix the port numbers on FEROLs and RUs")
        parser.add_argument("-a","--append",action='store_true',help="append measurements to data file")
        parser.add_argument("-o","--outputDir",default=self._evbTesterHome+'/log/',help="output directory [default: %(default)s]")
        parser.add_argument("--generateAtRU",action='store_true',help="ignore the FEROLs and generate data at the RU")
        parser.add_argument("--dropAtRU",action='store_true',help="drop data at the RU")
        parser.add_argument("--dropAtSocket",action='store_true',help="drop data at pt::blit socket callback")
        parser.add_argument("--ferolMode",action='store_true',help="generate data on FEROL instead of FRL")
        parser.add_argument("--scaleFedSizesFromFile",help="scale the FED fragment sizes relative to the sizes in the given file")
        parser.add_argument("--calculateFedSizesFromFile",help="calculate the FED fragment sizes using the parameters in the given file")
        parser.add_argument("--logLevel", type = str, default = None, help="set XDAQ log level")


    def fillFedSizeScalesFromFile(self):
        self.fedSizeScaleFactors = {}
        if self.args['scaleFedSizesFromFile']:
            with open(self.args['scaleFedSizesFromFile']) as file:
                for line in file:
                    (fedId,fedSize,fedSizeRMS,_) = line.split(',',3)
                    try:
                        self.fedSizeScaleFactors[fedId] = (0.,float(fedSize)/ConfigCase.defaultFedSize,0.,float(fedSizeRMS)/float(fedSize))
                    except ValueError:
                        pass
        elif self.args['calculateFedSizesFromFile']:
            with open(self.args['calculateFedSizesFromFile']) as file:
                for line in file:
                    try:
                        (fedIds,a,b,c,rms) = line.split(',',5)
                    except ValueError:
                        (fedIds,a,b,c,rms,_) = line.split(',')
                    try:
                        for id in fedIds.split('+'):
                            self.fedSizeScaleFactors[id] = (float(a),float(b),float(c),float(rms))
                    except ValueError:
                        pass


    def setup(self):
        if self.args['symbolMap']:
            self._symbolMap = SymbolMap(self.args['symbolMap'])
        else:
            configDir = os.path.dirname(configFile)
            self._symbolMap = SymbolMap(configDir+"/symbolMap.txt")
            self.startLaunchers()
            time.sleep(1)


    def doIt(self):
        if len(self.args['configs']) == 0:
            return False
        try:
            os.makedirs(self.args['outputDir'])
        except OSError:
            pass

        self.setup()
        self.fillFedSizeScalesFromFile()

        for configFile in self.args['configs']:
            configName = os.path.splitext(os.path.basename(configFile))[0]
            if os.path.isdir(configFile):
                configFile += "/"+configName+".xml"
            logFile = open(self.args['outputDir']+"/"+configName+".txt",'w')
            self.run(logfile,self.runConfig(configName,configFile))

        return True


    def runConfig(self,configName,configFile):
        try:
            allSizes = [256,512,768,1024,1149,1280,1408,1536,1664,1856,2048,2560,4096,8192,12288,16384]
            shortSizes = [256,512,768,1024,1280,1536,2048,2560,4096,8192,12288,16384]
            if self.args['relsizes']:
                sizes = [ int(r * ConfigCase.defaultFedSize) for r in self.args['relsizes'] ]
            elif self.args['full']:
                sizes = allSizes
            elif self.args['short']:
                sizes = shortSizes
            elif self.args['compl']:
                sizes = [ s for s in allSizes if s not in shortSizes ]
            else:
                # explicit list of fragment sizes specified on the command line
                sizes = self.args['sizes']

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
                if self.args['ferolMode']:
                    ferolMode = 'FEROL_MODE'
                else:
                    ferolMode = 'FRL_MODE'
                config = ConfigFromFile(self._symbolMap,configFile,self.args['fixPorts'],self.args['numa'],
                                            self.args['generateAtRU'],self.args['dropAtRU'],self.args['dropAtSocket'],ferolMode)
                configCase = ConfigCase(config,self._stdout,self.fedSizeScaleFactors)
                configCase.setXdaqLogLevel(self.args['logLevel'])
                configCase.prepare(configName)

                #----------
                # loop over specified fragment sizes
                #----------
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
            traceback.print_exc(file=self._stdout)
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
