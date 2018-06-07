#!/usr/bin/env python

import os
import re
import sys
import time
import traceback

from Configuration import ConfigFromFile, Configuration
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from ConfigCase import ConfigCase


class RunScans(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)

        # callback passed to TestCase to be called when startup is complete
        # and the number of measurements is zero
        self.afterStartupCallback = None

    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        TestRunner.addScanOptions(self,parser)
        parser.add_argument("configs",nargs='+',help="path to the config(s) to run")
        parser.add_argument("-m","--symbolMap",help="symbolMap file to use. This assumes that the launchers have been started.")
        parser.add_argument("--fixPorts",action='store_true',help="fix the port numbers on FEROLs and RUs")
        parser.add_argument("--generateAtRU",action='store_true',help="ignore the FEROLs and generate data at the RU")
        parser.add_argument("--dropAtRU",action='store_true',help="drop data at the RU")
        parser.add_argument("--dropAtSocket",action='store_true',help="drop data at pt::blit socket callback")
        parser.add_argument("--ferolMode",action='store_true',help="generate data on FEROL instead of FRL")
        parser.add_argument("--scaleFedSizesFromFile",help="scale the FED fragment sizes relative to the sizes in the given file")
        parser.add_argument("--calculateFedSizesFromFile",help="calculate the FED fragment sizes using the parameters in the given file")

    def setAfterStartupCallback(self, afterStartupCallback):
        self.afterStartupCallback = afterStartupCallback

    def fillFedSizeScalesFromFile(self):
        self.fedSizeScaleFactors = {}
        if self.args['scaleFedSizesFromFile']:
            with open(self.args['scaleFedSizesFromFile']) as file:
                for line in file:
                    (fedId,fedSize,fedSizeRMS,_) = line.split(',',3)
                    try:
                        self.fedSizeScaleFactors[fedId] = (0.,float(fedSize)/self.defaultFedSize,0.,float(fedSizeRMS)/float(fedSize))
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

    def getAllConfigurations(self):
        """:return a list of dicts with information about the tests to be run. Each entry of the returned
        value is a dict with the following keys:

        name           name of the configuration to be run
        symbolMap      a SymbolMap object
        config         a Configuration object
        configFile     the configuration file corresponding to the config object
        """

        result = []

        if self.args['ferolMode']:
            ferolMode = 'FEROL_MODE'
        else:
            ferolMode = 'FRL_MODE'

        for configFile in self.args['configs']:

            entry = {}

            #-----
            # configuration name
            #-----

            configName = os.path.splitext(os.path.basename(configFile))[0]
            if os.path.isdir(configFile):
                configFile += "/"+configName+".xml"

            entry['name'] = configName
            entry['configFile'] = configFile

            #-----
            # SymbolMap
            #-----

            if self.args['symbolMap']:
                # use symbol map specified on the command line
                # which is common to all test configuration
                symbolMap = SymbolMap(self.args['symbolMap'])
            else:
                # use test-specific SymbolMap
                configDir = os.path.dirname(configFile)
                symbolMap = SymbolMap(configDir+"/symbolMap.txt")

            entry['symbolMap'] = symbolMap

            #----------
            # create configuration
            #----------

            config = ConfigFromFile(symbolMap,configFile,self.args['fixPorts'],self.args['numa'],
                                    self.args['generateAtRU'],self.args['dropAtRU'],self.args['dropAtSocket'],ferolMode)

            entry['config'] = config

            result.append(entry)

        return result


    def doIt(self):
        if len(self.args['configs']) == 0:
            return False
        try:
            os.makedirs(self.args['outputDir'])
        except OSError:
            pass
        self.fillFedSizeScalesFromFile()


        for entry in self.getAllConfigurations():

            configName = entry['name']

            logFile = open(self.args['outputDir']+"/"+configName+".txt",'w')
            if self.args['verbose']:
                stdout = Tee(sys.stdout,logFile)
            else:
                startTime = time.strftime("%H:%M:%S", time.localtime())
                sys.stdout.write("%-32s: %s " % (configName,startTime))
                sys.stdout.flush()
                stdout = logFile

            config = entry['config']

            success = self.runConfig(configName,config,stdout)

            if not self.args['verbose']:
                stopTime = time.strftime("%H:%M:%S", time.localtime())
                print(stopTime+" "+success)
        return True


    def runConfig(self,configName,config,stdout, afterStartupCallback = None):

        assert isinstance(config, Configuration)

        try:
            # set self._symbolMap so that TestCase.startLaunchers() and TestCase.stopLaunchers()
            # know where the launchers are
            self._symbolMap = config.symbolMap

            if not self.args['symbolMap']:
                # test-specific SymbolMaps
                self.startLaunchers()
                time.sleep(1)

            #----------
            # run the configuration
            #----------

            configCase = ConfigCase(config,stdout,self.fedSizeScaleFactors,self.defaultFedSize,
                                    afterStartupCallback = self.afterStartupCallback)
            configCase.setXdaqLogLevel(self.args['logLevel'])
            configCase.prepare(configName)

            self.scanFedSizes(configName,configCase)

            returnValue = "\033[1;37;42m DONE \033[0m"
        except Exception as e:
            import logging
            logging.info("got an exception " + str(e) + " " + traceback.format_exc())
            traceback.print_exc(file=stdout)
            returnValue = "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        finally:
            configCase.destroy()
            if not self.args['symbolMap']:
                # test-specific SymbolMaps
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
