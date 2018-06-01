#!/usr/bin/env python

import os
import sys
import time
import traceback

from Configuration import Configuration
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from TestCase import TestCase
from Context import Context,RU,BU,RUBU


class RunBenchmarks(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)


    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        TestRunner.addScanOptions(self,parser)
        parser.add_argument("--nRUs",default=1,type=int,help="number of RUs, excl. EVM [default: %(default)s]")
        parser.add_argument("--nBUs",default=1,type=int,help="number of BUs [default: %(default)s]")
        parser.add_argument("--nRUBUs",default=0,type=int,help="number of RUBUs, excl. EVM [default: %(default)s]")
        try:
            symbolMapfile = self._evbTesterHome + '/cases/' + os.environ["EVB_SYMBOL_MAP"]
            parser.add_argument("-m","--symbolMap",default=symbolMapfile,help="symbolMap file to use, [default: %(default)s]")
        except KeyError:
            parser.add_argument("-m","--symbolMap",required=True,help="symbolMap file to use")


    def doIt(self):
        self._symbolMap = SymbolMap(self.args['symbolMap'])
        benchmark = "benchmark_"+str(self.args['nRUs'])+"x"+str(self.args['nBUs'])+"x"+str(self.args['nRUBUs'])

        logFile = open(self.args['logDir']+"/"+benchmark+".txt",'w',0)
        if self.args['verbose']:
            stdout = Tee(sys.stdout,logFile)
        else:
            startTime = time.strftime("%H:%M:%S", time.localtime())
            sys.stdout.write("%-32s: %s " % (benchmark,startTime))
            sys.stdout.flush()
            stdout = logFile

        try:
            self.startLaunchers()
            time.sleep(1)
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
        config = Configuration(self._symbolMap,self.args['numa'])
        # EVM
        config.add( RU(self._symbolMap,[
            ('inputSource','string','Local'),
            ('fedSourceIds','unsignedInt',(0,)),
            ('blockSize','unsignedInt','0x4000'),
            ('allocateBlockSize','unsignedInt','0x20000'),
            ('maxAllocateTime','unsignedInt','250'),
            ('numberOfResponders','unsignedInt','2'),
            ('socketBufferFIFOCapacity','unsignedInt','1024'),
            ('grantFIFOCapacity','unsignedInt','16384'),
            ('fragmentFIFOCapacity','unsignedInt','256'),
            ('fragmentRequestFIFOCapacity','unsignedInt','80')
            ]) )
        # RUs with 8 FEDs each
        for ru in range(self.args['nRUs']):
            config.add( RU(self._symbolMap,[
                ('inputSource','string','Local'),
                ('fedSourceIds','unsignedInt',range(8*ru+1,8*ru+9)),
                ('blockSize','unsignedInt','0x20000'),
                ('numberOfResponders','unsignedInt','6'),
                ('socketBufferFIFOCapacity','unsignedInt','1024'),
                ('grantFIFOCapacity','unsignedInt','16384'),
                ('fragmentFIFOCapacity','unsignedInt','256'),
                ('fragmentRequestFIFOCapacity','unsignedInt','6000')
                ]) )
        # BUs
        for bu in range(self.args['nBUs']):
            config.add( BU(self._symbolMap,[
                ('dropEventData','boolean','true'),
                ('lumiSectionTimeout','unsignedInt','0'),
                ('maxEvtsUnderConstruction','unsignedInt','640'),
                ('eventsPerRequest','unsignedInt','8'),
                ('superFragmentFIFOCapacity','unsignedInt','12800'),
                ('numberOfBuilders','unsignedInt','5')
                ]) )
        # RUBUs with 8 FEDs each
        for rubu in range(self.args['nRUBUs']):
            config.add( RUBU(self._symbolMap,
                [ # RU config
                ('inputSource','string','Local'),
                ('fedSourceIds','unsignedInt',range(8*rubu+1100,8*rubu+1108)), #avoid softFED
                ('blockSize','unsignedInt','0x20000'),
                ('numberOfResponders','unsignedInt','6'),
                ('socketBufferFIFOCapacity','unsignedInt','1024'),
                ('grantFIFOCapacity','unsignedInt','16384'),
                ('fragmentFIFOCapacity','unsignedInt','256'),
                ('fragmentRequestFIFOCapacity','unsignedInt','6000')
                ],
                [ #BU config
                ('dropEventData','boolean','true'),
                ('lumiSectionTimeout','unsignedInt','0'),
                ('maxEvtsUnderConstruction','unsignedInt','640'),
                ('eventsPerRequest','unsignedInt','8'),
                ('superFragmentFIFOCapacity','unsignedInt','12800'),
                ('numberOfBuilders','unsignedInt','5')
                ]) )
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
