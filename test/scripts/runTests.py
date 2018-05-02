#!/usr/bin/env python

import os
import re
import sys
import time
import traceback

from Configuration import Configuration
from TestRunner import TestRunner,Tee,BadConfig
from SymbolMap import SymbolMap
from TestCase import TestCase
import Context


class RunTests(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)

        self._testCaseDir = self._evbTesterHome + '/cases/'
        if not os.path.isdir(self._testCaseDir):
            print("Cannot find test case directory "+self._testCaseDir)
            sys.exit(2)

        sys.path.append(self._testCaseDir)


    def addOptions(self,parser):
        TestRunner.addOptions(self,parser)
        parser.add_argument("-a","--all",action='store_true',help="run all test cases in "+self._testCaseDir)
        parser.add_argument("tests",nargs='*',help="name of the test cases to run")
        try:
            symbolMapfile = self._testCaseDir + os.environ["EVB_SYMBOL_MAP"]
            parser.add_argument("-m","--symbolMap",default=symbolMapfile,help="symbolMap file to use, [default: %(default)s]")
        except KeyError:
            parser.add_argument("-m","--symbolMap",required=True,help="symbolMap file to use")


    def doIt(self):
        self._symbolMap = SymbolMap(self.args['symbolMap'])

        if self.args['launchers'] == 'start':
            self.startLaunchers()
            time.sleep(1)
        elif self.args['launchers'] == 'stop':
            self.stopLaunchers()
            return

        patterns = []
        if self.args['all']:
            patterns = [re.compile("^([0-9]x[0-9].*)\.py$")]
        elif self.args['tests']:
            for test in self.args['tests']:
                patterns.append(re.compile("("+test+")\.py$"))
        elif self.args['launchers']:
            return
        else:
            raise BadConfig

        testNames = []
        for file in os.listdir(self._testCaseDir):
            for pattern in patterns:
                testCase = pattern.search(file)
                if testCase:
                    testNames.append(testCase.group(1))
        testNames.sort()

        for test in testNames:
            logFile = open(self.args['logDir']+"/"+test+".txt",'w',0)
            self.runSetup(logFile,test,self.runTest,test)
        return


    def runTest(self,test):
        Context.resetInstanceNumbers()

        testModule = __import__(test,fromlist=test)
        testCase = getattr(testModule,'case_'+test)
        config = Configuration(self._symbolMap)
        case = testCase(config)
        case.fillConfiguration(self._symbolMap)
        case.prepare(test)
        case.runTest()


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    runTests = RunTests()
    runTests.addOptions(parser)
    try:
        runTests.run( parser.parse_args() )
    except BadConfig:
        parser.print_help()
