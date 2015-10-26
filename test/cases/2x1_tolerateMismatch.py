import operator
import time

from itertools import izip

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_tolerateMismatch(TestCase):

    runNumber = 0
    fedSizes = [1040,1248,1488,1696,1808,2160,2264,2280]

    def startEvB(self):
        self.runNumber += 1
        self.configureEvB()
        self.enableEvB(runNumber=self.runNumber)
        self.checkIt()


    def checkIt(self,missingFeds=[]):
        evmFEDs = [fed for fed in range(0,4) if fed not in missingFeds]
        if len(evmFEDs) < 4:
            self.checkAppState("MissingData","EVM")
        else:
            self.checkAppState("Enabled","EVM")

        ruFEDs  = [fed for fed in range(4,8) if fed not in missingFeds]
        if len(ruFEDs) < 4:
            self.checkAppState("MissingData","RU")
        else:
            self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")

        evmSize = sum([size for fed,size in enumerate(self.fedSizes) if fed in evmFEDs])
        ruSize  = sum([size for fed,size in enumerate(self.fedSizes) if fed in ruFEDs])
        self.checkEVM(evmSize)
        self.checkRU(ruSize)
        self.checkBU(evmSize+ruSize)

        if missingFeds:
            self.checkAppParam('nbEventsMissingData','unsignedLong',1000,operator.ge,"BU")
            dumps = self.getFiles("dump_run"+str(self.runNumber).zfill(6)+"_event[0-9]+_fed[0-9]+.txt$")
            if len(dumps) != 1:
                raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        else:
            self.checkAppParam('nbEventsMissingData','unsignedLong',0,operator.eq,"BU")


    def runTest(self):
        self.startEvB()
        print("Skipping an event on FED 2")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',2)
        time.sleep(7)
        self.checkIt([2])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate an event on FED 1")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',1)
        time.sleep(7)
        self.checkIt([1])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping an event on FED 5")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',5)
        time.sleep(7)
        self.checkIt([5])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate an event on FED 4")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',4)
        time.sleep(7)
        self.checkIt([4])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping an event on FED 0")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',0)
        time.sleep(7)
        self.checkIt([0])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate an event on FED 0")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',0)
        time.sleep(7)
        self.checkIt([0])
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id,[
                ('fedSize','unsignedInt',self.fedSizes[id])
                ]) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id,[
                ('fedSize','unsignedInt',self.fedSizes[id])
                ]) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
