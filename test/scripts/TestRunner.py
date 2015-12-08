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
        self._symbolMapfile = None

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
        self.parser.add_argument("-l","--launchers",choices=('start','stop'),help="start/stop xdaqLaunchers")
        if self._symbolMapfile:
            self.parser.add_argument("-m","--symbolMap",default=self._symbolMapfile,help="symbolMap file to use")
        else:
            self.parser.add_argument("-m","--symbolMap",required=True,help="symbolMap file to use")
        self.addOptions()
        self.args = vars( self.parser.parse_args() )
        self._symbolMap = SymbolMap(self.args['symbolMap'])
        if self.args['launchers'] == 'start':
            self.startLaunchers()
        elif self.args['launchers'] == 'stop':
            self.stopLaunchers()
        elif not self.doIt():
            self.parser.print_help()
            sys.exit()


    def startLaunchers(self):
        launcherCmd = "cd /tmp && sudo rm -f /tmp/core.* && export XDAQ_ROOT="+os.environ["XDAQ_ROOT"]+" && "+self._evbTesterHome+"/scripts/xdaqLauncher.py "
        if not self.args['verbose']:
            launcherCmd += "-l "+self._testLogDir+" "
        for launcher in self._symbolMap.launchers:
            print("Starting launcher on "+launcher[0]+":"+str(launcher[1]))
            subprocess.Popen(["ssh","-x","-n",launcher[0],launcherCmd+str(launcher[1])])


    def stopLaunchers(self):
        for launcher in self._symbolMap.launchers:
            print("Stopping launcher on "+launcher[0]+":"+str(launcher[1]))
            messengers.sendCmdToLauncher("stopLauncher",launcher[0],launcher[1])
