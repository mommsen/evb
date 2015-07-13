import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_crcErrors(TestCase):

    def checkIt(self,crcErrors=0):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',crcErrors,operator.eq,"BU")


    def runTest(self):
        self.configureEvB()
        self.enableEvB(sleepTime=1)
        self.checkIt()

        print("1 CRC error on FED 1")
        self.setAppParam('nbCRCerrors','unsignedInt','1','FEROL',1)
        time.sleep(2)
        print("3 CRC error on FED 5")
        self.setAppParam('nbCRCerrors','unsignedInt','3','FEROL',5)
        time.sleep(2)
        print("2 CRC error on FED 7")
        self.setAppParam('nbCRCerrors','unsignedInt','2','FEROL',7)
        time.sleep(3)
        self.checkState("Enabled")
        self.checkIt(6)
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed000[157].txt$")
        if len(dumps) != 6:
            raise ValueException("Expected 6 FED dump files, but found: "+str(dumps))
        dumps = self.getFiles("dump_run000001_event[0-9]+.txt$")
        if len(dumps) != 0:
            raise ValueException("Expected no event dump files, but found: "+str(dumps))

        print("100 CRC errors on FED 6")
        self.setAppParam('nbCRCerrors','unsignedInt','100','FEROL',6)
        time.sleep(5)
        self.checkState("Enabled")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0006.txt$")
        if len(dumps) != 10:
            raise ValueException("Expected 10 dump file from FED 6, but found: "+str(dumps))
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',10,operator.gt,"BU")

        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB()
        self.checkIt()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','1')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','1')
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
