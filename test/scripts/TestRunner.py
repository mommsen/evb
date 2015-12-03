#!/usr/bin/python

import argparse
import os
import subprocess
import sys
import time

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

    def __init__(self):
        self._testNames = []

        try:
            self._evbTesterHome = os.environ["EVB_TESTER_HOME"]
        except KeyError:
            print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
            sys.exit(2)

        self._testLogDir = self._evbTesterHome + '/log/'
        try:
            os.mkdir(self._testLogDir)
        except OSError:
            pass


    def run(self):
        self.parser = argparse.ArgumentParser()
        self.parser.add_argument("-v","--verbose",action='store_true',help="print log info to stdout")
        self.parser.add_argument("-l","--launchers",action='store_true',help="start xdaqLaunchers")
        self.parser.add_argument("-s","--stop",action='store_true',help="stop xdaqLaunchers")
        self.addOptions()
        self.args = vars( self.parser.parse_args() )
        print self.args
        if self.args['launchers']:
            self.startLaunchers()
        if self.args['stop']:
            self.stopLaunchers()
        self.doIt()


    def startLaunchers(self):
        launcherCmd = "cd /tmp && sudo rm -f /tmp/core.* && source "+self._evbTesterHome+"/setenv.sh && xdaqLauncher.py ";
        for launcher in self._symbolMap.launchers:
            print("Starting launcher on "+launcher[0]+":"+str(launcher[1]))
            subprocess.Popen(["ssh","-x","-n",launcher[0],launcherCmd+str(launcher[1])])


    def stopLaunchers(self):
        for launcher in self._symbolMap.launchers:
            print("Stopping launcher on "+launcher[0]+":"+str(launcher[1]))
            messengers.sendCmdToLauncher("stopLauncher",launcher[0],launcher[1])


    def runConfig(self,config):
        basename = os.path.basename(config)
        logFile = open(self._testLogDir+basename+".txt",'w')
        if self._verbose:
            stdout = Tee(sys.stdout,logFile)
        else:
            startTime = time.strftime("%H:%M:%S", time.localtime())
            sys.stdout.write("%-32s: %s " % (basename,startTime))
            sys.stdout.flush()
            stdout = logFile
