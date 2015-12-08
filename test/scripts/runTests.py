#!/usr/bin/python

import os
import re
import sys
import time
import traceback

from TestRunner import TestRunner,Tee
from SymbolMap import SymbolMap
from TestCase import TestCase


class RunTests(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)

        self._testCaseDir = self._evbTesterHome + '/cases/'
        if not os.path.isdir(self._testCaseDir):
            print("Cannot find test case directory "+self._testCaseDir)
            sys.exit(2)

        sys.path.append(self._testCaseDir)

        try:
            self._symbolMapfile = self._testCaseDir + os.environ["EVB_SYMBOL_MAP"]
        except KeyError:
            pass


    def addOptions(self):
        self.parser.add_argument("-a","--all",action='store_true',help="run all test cases in "+self._testCaseDir)
        self.parser.add_argument("tests",nargs='*',help="name of the test cases to run")


    def doIt(self):
        patterns = []
        if self.args['all']:
            patterns = [re.compile("^([0-9]x[0-9].*)\.py$")]
        elif self.args['tests']:
            for test in self.args['tests']:
                patterns.append(re.compile("("+test+")\.py$"))
        else:
            return False

        testNames = []
        for file in os.listdir(self._testCaseDir):
            for pattern in patterns:
                testCase = pattern.search(file)
                if testCase:
                    testNames.append(testCase.group(1))
        testNames.sort()

        for test in testNames:
            logFile = open(self._testLogDir+test+".txt",'w',0)
            if self.args['verbose']:
                stdout = Tee(sys.stdout,logFile)
            else:
                startTime = time.strftime("%H:%M:%S", time.localtime())
                sys.stdout.write("%-32s: %s " % (test,startTime))
                sys.stdout.flush()
                stdout = logFile

            success = self.runTest(test,stdout)

            if not self.args['verbose']:
                stopTime = time.strftime("%H:%M:%S", time.localtime())
                print(stopTime+" "+success)
        return True


    def runTest(self,test,stdout):
        try:
            testModule = __import__(test,fromlist=test)
            testCase = getattr(testModule,'case_'+test)
            case = testCase(self._symbolMap,stdout)
            try:
                case.run(test)
            except Exception as e:
                if self.args['verbose']:
                    traceback.print_exc(file=stdout)
                    raw_input("Press enter to continue")
                else:
                    raise e
        except Exception as e:
            traceback.print_exc(file=stdout)
            return "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        return "\033[1;37;42m Passed \033[0m"


if __name__ == "__main__":
    runTests = RunTests()
    runTests.run()
