#!/usr/bin/env python

import argparse
import os
import subprocess
import sys
import time

import messengers
from SymbolMap import SymbolMap
from TestCase import TestCase


class BadConfig(Exception):
    pass


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

    def __init__(self):
        try:
            self._evbTesterHome = os.environ["EVB_TESTER_HOME"]
        except KeyError:
            print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
            sys.exit(2)


    def addOptions(self,parser):
        parser.add_argument("-v","--verbose",action='store_true',help="print log info also to stdout")
        parser.add_argument("-l","--launchers",choices=('start','stop'),help="start/stop xdaqLaunchers")
        parser.add_argument("--logDir",default=self._evbTesterHome+'/log/',help="log directory [default: %(default)s]")


    def run(self,args):
        self.args = vars(args)
        #print(self.args)
        try:
            os.mkdir(self.args['logDir'])
        except OSError:
            pass
        self.doIt()


    def startLaunchers(self):
        launcherCmd = "cd /tmp && sudo rm -f /tmp/core.* && export XDAQ_ROOT="+os.environ["XDAQ_ROOT"]+" && "+self._evbTesterHome+"/scripts/xdaqLauncher.py "
        if not self.args['verbose']:
            launcherCmd += "-l "+self.args['logDir']+" "
        for launcher in self._symbolMap.launchers:
            if self.args['verbose']:
                print("Starting launcher on "+launcher[0]+":"+str(launcher[1]))
            subprocess.Popen(["ssh","-x","-n",launcher[0],launcherCmd+str(launcher[1])])


    def stopLaunchers(self):
        for launcher in self._symbolMap.launchers:
            if self.args['verbose']:
                print("Stopping launcher on "+launcher[0]+":"+str(launcher[1]))
            messengers.sendCmdToLauncher("stopLauncher",launcher[0],launcher[1])
