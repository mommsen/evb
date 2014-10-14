import glob
import operator
import os
import shutil
import sys
import time

from TestCase import TestCase
from Configuration import RU,BU


class case_2x1_write(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(sleepTime=15,runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        sys.stdout.write("Stopping EVM")
        sys.stdout.flush()
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
        eventCount = self.getAppParam('eventCount','unsignedLong','EVM')
        eventCount.update( self.getAppParam('nbEventsBuilt','unsignedLong','BU') )
        if eventCount['EVM0'] != eventCount['BU0']:
            raise ValueError("EVM counted "+str(eventCount['EVM0'])+" events, while BU built "+str(eventCount['BU0'])+" events")
        self.checkBuDir(testDir,runNumber,eventSize=28672)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ]) )
        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6')
            ]) )