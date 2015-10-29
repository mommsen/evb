import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_corruptedEvents_ptfrl(TestCase):

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
        time.sleep(5)
        self.checkAppState("MissingData","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkEVM(6144)
        self.checkIt()
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0002.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 2, but found: "+str(dumps))

        print("Corrupt 2 events on FED 7 - not tolerating")
        self.setAppParam('corruptNbEvents','unsignedInt','2','FEROL',7)
        time.sleep(3)
        self.checkAppState("MissingData","EVM")
        self.checkAppState("Failed","RU")
        self.checkAppState("Enabled","BU")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0007.txt$")
        if len(dumps) != 2:
            raise ValueException("Expected 2 dump files from FED 7, but found: "+str(dumps))

        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('checkCRC','unsignedInt','0'),
             ('tolerateCorruptedEvents','boolean','true')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('checkCRC','unsignedInt','0'),
             ('tolerateCorruptedEvents','boolean','false')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('checkCRC','unsignedInt','1'),
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
