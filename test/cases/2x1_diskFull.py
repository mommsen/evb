import glob
import operator
import os
import shutil
import sys
import time

from TestCase import TestCase
from Configuration import RU,BU


class case_2x1_diskFull(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        diskUsage = self.prepareAppliance(testDir)
        self.setAppParam('rawDataLowWaterMark','double',str(diskUsage),'BU')
        self.setAppParam('rawDataHighWaterMark','double',str(diskUsage+0.0005),'BU')
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(runNumber=runNumber)
        self.checkAppParam('eventRate','unsignedInt',500,operator.gt,"BU")

        sys.stdout.write("Running until disk is full...")
        sys.stdout.flush()
        self.waitForAppParam('eventRate','unsignedInt',0,operator.eq,'EVM')
        print(" done")
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        #self.checkAppState("Blocked","BU")
        self.checkAppState("Enabled","BU")
        print("Freeing disk space")
        for rawFile in glob.glob(runDir+"/*.raw"):
            os.remove(rawFile)
        time.sleep(5)
        self.checkState("Enabled")
        self.checkAppParam('eventRate','unsignedInt',500,operator.gt,"BU")
        self.stopEvB()
        eventCount = self.getAppParam('eventCount','unsignedLong','EVM')
        eventCount.update( self.getAppParam('nbEventsBuilt','unsignedLong','BU') )
        if eventCount['EVM0'] != eventCount['BU0']:
            raise ValueError("EVM counted "+str(eventCount['EVM0'])+" events, while BU built "+str(eventCount['BU0'])+" events")
        self.checkBuDir(testDir,runNumber)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(4)),
             ('useLogNormal','boolean','true'),
             ('dummyFedSize','unsignedInt','10000'),
             ('dummyFedSizeStdDev','unsignedInt','1000'),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('useLogNormal','boolean','true'),
             ('dummyFedSize','unsignedInt','10000'),
             ('dummyFedSizeStdDev','unsignedInt','1000')
            ]) )
        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6')
            ]) )
