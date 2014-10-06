#!/usr/bin/python

import getopt
import os
import pkgutil
import re
import socket
import sys
from time import sleep

import messengers
from SymbolMap import SymbolMap
from TestCase import TestCase

def usage():
    print("""
testRunner.py --test testCase
          -h --help:         print this message and exit
          -t --test:         name of the test case
    """)


def main(argv):

    testNames = []

    try:
        testCaseDir = os.environ["EVB_TESTER_HOME"] + '/cases/'
    except KeyError:
        print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
        sys.exit(2)

    if not os.path.isdir(testCaseDir):
        print("Cannot find test case directory "+testCaseDir)
        sys.exit(2)

    sys.path.append(testCaseDir)

    symbolMap = SymbolMap(testCaseDir)

    try:
        opts,args = getopt.getopt(argv,"t:",["test="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            usage()
            sys.exit()
        elif opt in ("-t", "--test"):
            testNames.append(arg)

    for test in testNames:
        testModule = __import__(test,fromlist=test)
        testCase = getattr(testModule,'case_'+test)
        case = testCase(symbolMap)
        case.run()


if __name__ == "__main__":
    main(sys.argv[1:])
