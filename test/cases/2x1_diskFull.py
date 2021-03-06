import glob
import operator
import os
import shutil
import sys
import time

from TestCase import *
from Context import RU,BU


class case_2x1_diskFull(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        diskUsage = self.prepareAppliance(testDir,runNumber)
        self.setAppParam('rawDataLowWaterMark','double',str(diskUsage),'BU')
        self.setAppParam('rawDataHighWaterMark','double',str(diskUsage+0.1),'BU')
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(runNumber=runNumber,sleepTime=0)
        sys.stdout.write("Running until disk is full...")
        sys.stdout.flush()
        self.waitForAppParam('eventRate','unsignedInt',0,operator.eq,'EVM')
        # avoid some spurious wake ups
        time.sleep(3)
        self.waitForAppParam('eventRate','unsignedInt',0,operator.eq,'EVM')
        print(" done")
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Blocked","BU")
        print("Freeing disk space")
        for rawFile in glob.glob(runDir+"/*.raw"):
            os.remove(rawFile)
        time.sleep(2)
        try:
            self.checkAppState("Enabled","BU")
        except StateException:
            self.checkAppState("Throttled","BU")
        self.checkAppParam('eventRate','unsignedInt',500,operator.gt,"BU")
        self.stopEvB(runDir)
        self.checkBuDir(testDir,runNumber)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),
             ('fakeLumiSectionDuration','unsignedInt','5'))
             + self.getFedParams(range(4),10000,1000)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(7,20),10000,1000)
            ) )
        self._config.add( BU(symbolMap,(
             ('lumiSectionTimeout','unsignedInt','10'),
             ('staleResourceTime','unsignedInt','0'),
             ('sleepTimeBlocked','unsignedInt','1')
            )) )
