import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_corruptedEvents(TestCase):

    def checkIt(self):
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',0,operator.eq,"BU")


    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()

        print("Corrupt an event on FED 2 - tolerating")
        self.setAppParam('corruptNbEvents','unsignedInt','1','FEROL',2)
        time.sleep(3)
        self.checkState("Enabled")
        self.checkEVM(8192)
        self.checkIt()
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0002.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 2, but found: "+str(dumps))

        print("Corrupt 2 events on FED 7 - not tolerating")
        self.setAppParam('corruptNbEvents','unsignedInt','2','FEROL',7)
        time.sleep(3)
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Failed","RU")
        self.checkAppState("Enabled","BU")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0007.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 7, but found: "+str(dumps))

        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()


    def fillConfiguration(self,symbolMap):
        evmFedIds = range(4)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('checkCRC','unsignedInt','1'),
             ('tolerateCorruptedEvents','boolean','true')
            ],evmFedIds)
        for id in evmFedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )

        ruFedIds = range(4,8)
        ru = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('checkCRC','unsignedInt','1'),
             ('tolerateCorruptedEvents','boolean','false')
            ],ruFedIds)
        for id in ruFedIds:
            self._config.add( FEROL(symbolMap,ru,[
                ('fedId','unsignedInt',str(id))
                ]) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('checkCRC','unsignedInt','1'),
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
