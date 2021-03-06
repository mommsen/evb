import glob
import operator
import os
import shutil
import sys
import time

from TestCase import TestCase
from Context import RU,BU


class case_4x1_write(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir,runNumber)
        print("Emulating some left-over runs")
        dummyRuns = ('123456','234567','345678')
        for run in dummyRuns:
            os.makedirs(testDir+"/run"+run)
        with open(testDir+"/run234567/run234567_ls0000_EoR.jsn",'w') as dummy:
            dummy.write("")
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(sleepTime=15,runNumber=runNumber)
        self.checkEVM(16384)
        self.checkRU(16384)
        self.checkBU(65536)
        self.stopEvB()
        self.checkBuDir(testDir,runNumber,eventSize=65560)
        for run in dummyRuns:
            eorFile = testDir+"/run"+run+"/run"+run+"_ls0000_EoR.jsn"
            if not os.path.isfile(eorFile):
                raise FileException(eorFile+" does not exist")


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),
             ('fakeLumiSectionDuration','unsignedInt','5'))
             + self.getFedParams(range(0,16),1024)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(16,32),1024)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(32,48),1024)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(48,64),1024)
            ) )
        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6'),
             ('staleResourceTime','unsignedInt','0'),
             ('numberOfBuilders','unsignedInt','2')
            ]) )
