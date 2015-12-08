#!/usr/bin/python

import os
import re
import sys
import time
import traceback

from TestRunner import TestRunner,Tee
from SymbolMap import SymbolMap
from Config import Config


class RunConfigs(TestRunner):

    def __init__(self):
        TestRunner.__init__(self)


    def addOptions(self):
        self.parser.add_argument("configs",nargs='+',help="path to the config(s) to run")


    def doIt(self):
        if len(self.args['configs']) == 0:
            return False
        for configFile in self.args['configs']:
            config = os.path.splitext(os.path.basename(configFile))[0]
            logFile = open(self._testLogDir+config+".txt",'w')
            if self.args['verbose']:
                stdout = Tee(sys.stdout,logFile)
            else:
                startTime = time.strftime("%H:%M:%S", time.localtime())
                sys.stdout.write("%-32s: %s " % (config,startTime))
                sys.stdout.flush()
                stdout = logFile

            success = self.runConfig(configFile,stdout)

            if not self.args['verbose']:
                stopTime = time.strftime("%H:%M:%S", time.localtime())
                print(stopTime+" "+success)
        return True


    def runConfig(self,configFile,stdout):
        try:
            config = Config(self._symbolMap,configFile,stdout)
            config.run()
        except Exception as e:
            traceback.print_exc(file=stdout)
            return "\033[1;37;41m FAILED \033[0m "+type(e).__name__+": "+str(e)
        return "\033[1;37;42m Passed \033[0m"


if __name__ == "__main__":
    runConfigs = RunConfigs()
    runConfigs.run()
