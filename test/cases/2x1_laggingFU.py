import glob
import operator
import os
import shutil
import sys
import time

from TestCase import *
from Configuration import RU,BU


class case_2x1_laggingFU(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir,runNumber,activeRunNumQueuedLS=3)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.configureEvB()
        try:
            self.enableEvB(runNumber=runNumber)
        except StateException:
            self.checkAppState("Blocked","BU",0)
        else:
            raise StateException("EvB should not be Enabled")
        self.haltEvB()
        self.setAppParam('maxFuLumiSectionLatency','unsignedInt','4','BU')
        self.configureEvB()
        runNumber=time.strftime("%s",time.localtime())
        self.enableEvB(runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        self.stopEvB()
        self.checkBuDir(testDir,runNumber,eventSize=28672)


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
             ('maxFuLumiSectionLatency','unsignedInt','2'),
             ('staleResourceTime','unsignedInt','0')
            ]) )
