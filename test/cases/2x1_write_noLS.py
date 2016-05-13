import glob
import operator
import os
import shutil
import sys
import time

from TestCase import TestCase
from Context import RU,BU


class case_2x1_write_noLS(TestCase):

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
        self.stopEvB()
        self.checkBuDir(testDir,runNumber,eventSize=26648)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','0')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','0'),
             ('staleResourceTime','unsignedInt','0'),
             ('fuBlacklist','string','[fu-c2e07-18-01.cms, fu-c2e07-24-01.cms]')
            ]) )
