#!/usr/bin/python

import getopt
import os
import re
import sys
import time
import traceback

import messengers
from SymbolMap import SymbolMap
from TestCase import TestCase

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

    def __init__(self,argv):
        self._verbose = False
        self._testNames = []

        try:
            evbTesterHome = os.environ["EVB_TESTER_HOME"]
        except KeyError:
            print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
            sys.exit(2)

        self._testLogDir = evbTesterHome + '/log/'
        try:
            os.mkdir(self._testLogDir)
        except OSError:
            pass

        self._testCaseDir = evbTesterHome + '/cases/'
        if not os.path.isdir(self._testCaseDir):
            print("Cannot find test case directory "+self._testCaseDir)
            sys.exit(2)

        sys.path.append(self._testCaseDir)
        self._symbolMap = SymbolMap(self._testCaseDir)

        self.parseArgs(argv)


    def usage(self):
        print("""
testRunner.py --test testCase
           -h --help:         print this message and exit
           -t --test:         name of the test case
           -a --all:          run all tests
           -v --verbose:      print log info to stdout
""")


    def parseArgs(self,argv):

        pattern = None

        try:
            opts,args = getopt.getopt(argv,"hvat:",["help","verbose","all","test="])
        except getopt.GetoptError:
            usage()
            sys.exit(2)
        for opt, arg in opts:
            if opt == '-h':
                usage()
                sys.exit()
            elif opt in ("-v","--verbose"):
                self._verbose = True
            elif opt in ("-t","--test"):
                pattern = re.compile("("+arg+")\.py$")
            elif opt in ("-a","--all"):
                pattern = re.compile("^([0-9]x[0-9].*)\.py$")

        if pattern is None:
            print("Please specify a test to run")
            sys.exit(2)

        for file in os.listdir(self._testCaseDir):
            testCase = pattern.search(file)
            if testCase:
                self._testNames.append(testCase.group(1))
        self._testNames.sort()


    def runTest(self,test,stdout):
        try:
            testModule = __import__(test,fromlist=test)
            testCase = getattr(testModule,'case_'+test)
            case = testCase(self._symbolMap,stdout)
            try:
                case.run()
            except Exception as e:
                if self._verbose:
                    traceback.print_exc(file=stdout)
                    raw_input("Press enter to continue")
                else:
                    raise e
        except Exception as e:
            traceback.print_exc(file=stdout)
            return "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        return "\033[1;37;42m Passed \033[0m"


    def doIt(self):
        for test in self._testNames:
            logFile = open(self._testLogDir+test+".txt",'w')
            if self._verbose:
                stdout = Tee(sys.stdout,logFile)
            else:
                startTime = time.strftime("%H:%M:%S", time.localtime())
                sys.stdout.write("%-32s: %s " % (test,startTime))
                sys.stdout.flush()
                stdout = logFile

            success = self.runTest(test,stdout)

            if not self._verbose:
                stopTime = time.strftime("%H:%M:%S", time.localtime())
                print(stopTime+" "+success)



if __name__ == "__main__":
    testRunner = TestRunner(sys.argv[1:])
    testRunner.doIt()
