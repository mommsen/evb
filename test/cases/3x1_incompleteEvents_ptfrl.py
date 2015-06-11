import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_3x1_incompleteEvents_ptfrl(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir,runNumber)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(sleepTime=15,runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(6144)
        self.checkBU(14336)

        print("Skipping an event on FED 3")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',3)
        time.sleep(5)
        self.checkAppState("SyncLoss","RU",1)
        self.checkAppState("Enabled","RU",2)
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")

        # Wait for ls timeouts
        time.sleep(10)

        self.haltEvB()
        self.checkBuDir(testDir,runNumber,eventSize=24576)


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ])
        for id in range(0,1):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru1 = RU(symbolMap,[
             ('inputSource','string','FEROL'),
            ])
        for id in range(1,4):
            self._config.add( FEROL(symbolMap,ru1,id) )

        ru2 = RU(symbolMap,[
             ('inputSource','string','FEROL'),
            ])
        for id in range(4,7):
            self._config.add( FEROL(symbolMap,ru2,id) )

        self._config.add( evm )
        self._config.add( ru1 )
        self._config.add( ru2 )

        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6'),
             ('staleResourceTime','unsignedInt','0')
            ]) )
