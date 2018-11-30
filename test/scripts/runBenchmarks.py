#!/usr/bin/env python

import os
import sys
import time
import traceback

from Configuration import Configuration
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from TestCase import TestCase
from Context import Context,RU,BU,RUBU, resetInstanceNumbers


class RunBenchmarks(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)
        self._symbolMap = None

    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        TestRunner.addScanOptions(self,parser)
        parser.add_argument("--foldedEVM",action='store_true',help="run a BU on the EVM node [default: %(default)s]")
        parser.add_argument("--canonicalEVM",action='store_true',help="emulate 7 additional FEDs on EVM [default: %(default)s]")
        parser.add_argument("--nRUs",default=1,type=int,help="number of RUs, excl. EVM [default: %(default)s]")
        parser.add_argument("--nBUs",default=1,type=int,help="number of BUs [default: %(default)s]")
        parser.add_argument("--nRUBUs",default=0,type=int,help="number of RUBUs, excl. EVM [default: %(default)s]")
        parser.add_argument("--outputDisk",help="full path to output directory. If not specified, the data is dropped on the BU")
        try:
            symbolMapfile = self._evbTesterHome + '/cases/' + os.environ["EVB_SYMBOL_MAP"]
            parser.add_argument("-m","--symbolMap",default=symbolMapfile,help="symbolMap file to use, [default: %(default)s]")
        except KeyError:
            parser.add_argument("-m","--symbolMap",required=True,help="symbolMap file to use")


    def createSymbolMap(self):
        if self._symbolMap is None:
            self._symbolMap = SymbolMap(self.args['symbolMap'])


    def getAllConfigurations(self):
        """:return a list of dicts with information about the tests to be run. Each entry of the returned
        value is a dict with the following keys:

        name           name of the configuration to be run
        symbolMap      a SymbolMap object
        config         a Configuration object
        """

        self.createSymbolMap()

        return [
            dict(
                name = self.getBenchmarkName(),
                symbolMap = self._symbolMap,
                config = self.getConfiguration(),
            )
        ]


    def getBenchmarkName(self):
        return "benchmark_"+str(self.args['nRUs'])+"x"+str(self.args['nBUs'])+"x"+str(self.args['nRUBUs'])

    def doIt(self):
        self.createSymbolMap()
        benchmark = self.getBenchmarkName()

        logFile = open(self.args['outputDir']+"/"+benchmark+".txt",'w',0)
        if self.args['verbose']:
            stdout = Tee(sys.stdout,logFile)
        else:
            startTime = time.strftime("%H:%M:%S", time.localtime())
            sys.stdout.write("%-32s: %s " % (benchmark,startTime))
            sys.stdout.flush()
            stdout = logFile

        try:
            self.startLaunchers()
            time.sleep(5)
            self.runBenchmark(benchmark,stdout)
            success = "\033[1;37;42m OKAY \033[0m"
        except Exception as e:
            traceback.print_exc(file=stdout)
            success = "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        finally:
            self.stopLaunchers()

        if not self.args['verbose']:
            stopTime = time.strftime("%H:%M:%S", time.localtime())
            print(stopTime+" "+success)


    def getConfiguration(self):
        """Note that this needs the xdaq launchers running since it has to query the remote hosts
        to get numa information based on the output of numactl --hardware"""
        resetInstanceNumbers()

        evmConfig = [
            ('inputSource','string','Local'),
            ('allocateBlockSize','unsignedInt','0x20000'),
            ('maxAllocateTime','unsignedInt','250'),
            ('socketBufferFIFOCapacity','unsignedInt','1024'),
            ('grantFIFOCapacity','unsignedInt','16384'),
            ('fragmentFIFOCapacity','unsignedInt','256'),
            ('fragmentRequestFIFOCapacity','unsignedInt','80')
            ]
        if self.args['canonicalEVM']:
            evmConfig.append( ('fedSourceIds','unsignedInt',range(1000,1008)) )
            evmConfig.append( ('blockSize','unsignedInt','0x3fff0') )
            evmConfig.append( ('numberOfResponders','unsignedInt','6') )
        else:
            evmConfig.append( ('fedSourceIds','unsignedInt',(1000,)) )
            evmConfig.append( ('blockSize','unsignedInt','0x4000') )
            evmConfig.append( ('numberOfResponders','unsignedInt','2') )
        ruConfig = [
            ('inputSource','string','Local'),
            ('blockSize','unsignedInt','0x3fff0'),
            ('numberOfResponders','unsignedInt','6'),
            ('socketBufferFIFOCapacity','unsignedInt','1024'),
            ('grantFIFOCapacity','unsignedInt','16384'),
            ('fragmentFIFOCapacity','unsignedInt','256'),
            ('fragmentRequestFIFOCapacity','unsignedInt','6000')
            ]
        buConfig = [
            ('lumiSectionTimeout','unsignedInt','0'),
            ('maxEvtsUnderConstruction','unsignedInt','320'),
            ('eventsPerRequest','unsignedInt','8'),
            ('superFragmentFIFOCapacity','unsignedInt','12800'),
            ('numberOfBuilders','unsignedInt','5')
            ]
        if self.args['outputDisk']:
            buConfig.append( ('dropEventData','boolean','false') )
            buConfig.append( ('rawDataDir','string',self.args['outputDisk']) )
            buConfig.append( ('metaDataDir','string',self.args['outputDisk']) )
            buConfig.append( ('deleteRawDataFiles','boolean','true') )
            buConfig.append( ('ignoreResourceSummary','boolean','true') )
            buConfig.append( ('maxEventsPerFile','unsignedInt','100') )
        else:
            buConfig.append( ('dropEventData','boolean','true') )

        config = Configuration(self._symbolMap,self.args['numa'])
        # EVM
        if self.args['foldedEVM']:
            config.add( RUBU(self._symbolMap,evmConfig,buConfig) )
        else:
            config.add( RU(self._symbolMap,evmConfig) )
        # RUs with 8 FEDs each
        for ru in range(self.args['nRUs']):
            config.add( RU(self._symbolMap,
                               ruConfig +
                               [('fedSourceIds','unsignedInt',range(8*ru,8*ru+8)),]
                               ) )
        # BUs
        for bu in range(self.args['nBUs']):
            config.add( BU(self._symbolMap,buConfig) )
        # RUBUs with 8 FEDs each
        for rubu in range(self.args['nRUBUs']):
            config.add( RUBU(self._symbolMap,
                                 ruConfig +
                                 [('fedSourceIds','unsignedInt',range(8*rubu+1100,8*rubu+1108)),] #avoid softFED
                                 , buConfig
                                 ) )
        return config


    def runBenchmark(self,benchmark,stdout):
        config = self.getConfiguration()
        testCase = TestCase(config,stdout)
        testCase.prepare(benchmark)

        self.scanFedSizes(benchmark,testCase)
        testCase.destroy()


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    runBenchmarks = RunBenchmarks()
    runBenchmarks.addOptions(parser)
    try:
        runBenchmarks.run( parser.parse_args() )
    except BadConfig:
        parser.print_help()
