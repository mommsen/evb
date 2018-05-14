#!/usr/bin/env python

import os
import sys
import time
import traceback

from Configuration import Configuration
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from TestCase import TestCase
from Context import Context,RU,BU


class RunBenchmarks(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)


    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        TestRunner.addScanOptions(self,parser)
        parser.add_argument("--nRUs",default=1,type=int,help="number of RUs [default: %(default)s]")
        parser.add_argument("--nBUs",default=1,type=int,help="number of BUs [default: %(default)s]")
        try:
            symbolMapfile = self._evbTesterHome + '/cases/' + os.environ["EVB_SYMBOL_MAP"]
            parser.add_argument("-m","--symbolMap",default=symbolMapfile,help="symbolMap file to use, [default: %(default)s]")
        except KeyError:
            parser.add_argument("-m","--symbolMap",required=True,help="symbolMap file to use")


    def doIt(self):

        benchmark = "benchmark_"+str(self.args['nRUs'])+"x"+str(self.args['nBUs'])

        logFile = open(self.args['logDir']+"/"+benchmark+".txt",'w',0)
        if self.args['verbose']:
            stdout = Tee(sys.stdout,logFile)
        else:
            startTime = time.strftime("%H:%M:%S", time.localtime())
            sys.stdout.write("%-32s: %s " % (benchmark,startTime))
            sys.stdout.flush()
            stdout = logFile

        try:
            self.runBenchmark(benchmark,stdout)
            success = "\033[1;37;42m OKAY \033[0m"
        except Exception as e:
            traceback.print_exc(file=stdout)
            success = "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)

        if not self.args['verbose']:
            stopTime = time.strftime("%H:%M:%S", time.localtime())
            print(stopTime+" "+success)


    def getConfiguration(self):
        symbolMap = SymbolMap(self.args['symbolMap'])
        config = Configuration(symbolMap,self.args['numa'])
        # EVM
        config.add( RU(symbolMap,[
            ('inputSource','string','Local'),
            ('fedSourceIds','unsignedInt',(0,))
            ]) )
        # RUs with 8 FEDs each
        for ru in range(self.args['nRUs']):
            config.add( RU(symbolMap,[
                ('inputSource','string','Local'),
                ('fedSourceIds','unsignedInt',range(8*ru+1,8*ru+9))
                ]) )
        # BUs
        for bu in range(self.args['nBUs']):
            config.add( BU(symbolMap,[
                ('dropEventData','boolean','true'),
                ('lumiSectionTimeout','unsignedInt','0')
                ]) )
        return config


    def runBenchmark(self,benchmark,stdout):
        config = self.getConfiguration()
        testCase = TestCase(config,stdout)
        testCase.prepare(benchmark)

        self.scanFedSizes(benchmark,testCase)
        del(testCase)


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    runBenchmarks = RunBenchmarks()
    runBenchmarks.addOptions(parser)
    try:
        runBenchmarks.run( parser.parse_args() )
    except BadConfig:
        parser.print_help()
