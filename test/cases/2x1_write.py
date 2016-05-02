import glob
import operator
import os
import shutil
import sys
import time

from TestCase import TestCase
from Context import RU,BU


class case_2x1_write(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        self.prepareAppliance(testDir,runNumber)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(sleepTime=15,runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)

        eventRate = self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        lastEvent = self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0']
        eventToStop = lastEvent + max(2*eventRate,1000)
        sys.stdout.write("Stopping EvB at event "+str(eventToStop-1))
        sys.stdout.flush()
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'EVM')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'RU')
        self.stop('EVM')
        self.waitForAppState('Ready','EVM',maxTries=20)
        print(" done")
        sys.stdout.write("Wait for lumi section timeouts...")
        sys.stdout.flush()
        time.sleep(15)
        print(" done")
        self.stop('RU')
        self.stop('BU')
        sys.stdout.write("Stopping EvB")
        self.waitForState('Ready')
        print(" done")
        self.checkBuDir(testDir,runNumber,eventSize=26648)

        runNumber=time.strftime("%s",time.localtime())
        self.enableEvB(sleepTime=15,runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        self.stopEvB()
        self.checkBuDir(testDir,runNumber,eventSize=26648)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6'),
             ('staleResourceTime','unsignedInt','0'),
             ('fuBlacklist','string','[fu-c2e07-18-01.cms, fu-c2e07-24-01.cms]')
            ]) )
