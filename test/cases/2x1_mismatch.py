import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_mismatch(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)


    def runTest(self):
        self.configureEvB()
        self.enableEvB(runNumber=1)
        self.checkIt()

        print("Skipping an event on FED 2")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',2)
        time.sleep(5)
        self.checkAppState("SyncLoss","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0002.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()
        time.sleep(1)

        self.configureEvB()
        self.enableEvB(runNumber=2)
        self.checkIt()

        print("Duplicate an event on FED 0")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',0)
        time.sleep(5)
        self.checkAppState("SyncLoss","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        dumps = self.getFiles("dump_run000002_event[0-9]+_fed0000.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()
        time.sleep(1)

        self.configureEvB()
        self.enableEvB(runNumber=3)
        self.checkIt()

        print("Skipping an event on FED 5")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',5)
        time.sleep(5)
        self.checkAppState("SyncLoss","RU")
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        dumps = self.getFiles("dump_run000003_event[0-9]+_fed0005.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()
        time.sleep(1)

        self.configureEvB()
        self.enableEvB(runNumber=4)
        self.checkIt()

        print("Duplicate an event on FED 4")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',4)
        time.sleep(5)
        self.checkAppState("SyncLoss","RU")
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        dumps = self.getFiles("dump_run000004_event[0-9]+_fed0004.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evmFedIds = range(4)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ],evmFedIds)
        for id in evmFedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )

        ruFedIds = range(4,8)
        ru = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ],ruFedIds)
        for id in ruFedIds:
            self._config.add( FEROL(symbolMap,ru,[
                ('fedId','unsignedInt',str(id))
                ]) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
