import operator
import time

from itertools import izip

from TestCase import *
from Context import FEROL,RU,BU


class case_2x1_tolerateMismatch(TestCase):

    runNumber = 0
    fedSizes = [1040,1248,1488,1696,1808,2160,2264,2280]

    def startEvB(self):
        self.runNumber += 1
        try:
            self.configureEvB()
        except StateException:
            print(" skipped")
        self.enableEvB(runNumber=self.runNumber)
        self.checkIt()


    def checkIt(self):
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        evmSize = sum(self.fedSizes[:4])
        ruSize  = sum(self.fedSizes[4:])
        self.checkEVM(evmSize)
        self.checkRU(ruSize)
        self.checkBU(evmSize+ruSize)


    def checkMissing(self,missingFeds,nbDumps=None):
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

        self.checkAppParam('nbEventsMissingData','unsignedLong',1000,operator.ge,"BU")
        dumps = self.getFiles("dump_run"+str(self.runNumber).zfill(6)+"_event[0-9]+_fed[0-9]+.txt$")
        if nbDumps is None:
            nbDumps = len(missingFeds)
        if len(dumps) != nbDumps:
            raise ValueException("Expected "+str(nbDumps)+" FED dump files, but found: "+str(dumps))


    def runTest(self):
        self.startEvB()
        print("Skipping 6 events on FED 2")
        self.setAppParam('skipNbEvents','unsignedInt','6','FEROL',2)
        time.sleep(7)
        self.checkMissing([2])
        self.sendResync()
        self.checkIt()
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate 9 events on FED 1")
        self.setAppParam('duplicateNbEvents','unsignedInt','9','FEROL',1)
        time.sleep(7)
        self.checkMissing([1])
        self.sendResync()
        self.checkIt()
        self.stopEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping 3 events on FED 5")
        self.setAppParam('skipNbEvents','unsignedInt','3','FEROL',5)
        time.sleep(7)
        self.checkMissing([5])
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate 4 events on FED 6")
        self.setAppParam('duplicateNbEvents','unsignedInt','4','FEROL',6)
        time.sleep(7)
        self.checkMissing([6])
        self.stopEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping 5 events on FED 7")
        self.setAppParam('skipNbEvents','unsignedInt','5','FEROL',7)
        time.sleep(7)
        self.checkMissing([7])
        self.stopEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping an event on FED 3")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',3)
        time.sleep(7)
        self.checkMissing([3])
        self.stopEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate an event on FED 4")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',4)
        time.sleep(7)
        self.checkMissing([4])
        print("Skipping 7 events on FED 6")
        self.setAppParam('skipNbEvents','unsignedInt','7','FEROL',6)
        time.sleep(7)
        self.checkMissing([4,6])
        self.sendResync()
        self.checkIt()
        self.stopEvB()
        time.sleep(1)

        self.startEvB()
        print("Skipping events on FED 2 and 7")
        self.setAppParam('skipNbEvents','unsignedInt','7','FEROL',7)
        self.setAppParam('skipNbEvents','unsignedInt','3','FEROL',2)
        time.sleep(7)
        self.checkMissing([2,7])
        self.sendResync()
        self.checkIt()
        print("Skipping an event on FED 5 and 6")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',5)
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',6)
        time.sleep(7)
        self.checkMissing([5,6],4)
        self.sendResync()
        self.checkIt()
        self.haltEvB()
        time.sleep(1)

        # Errors on the master FED should not be tolerated
        self.startEvB()
        print("Skipping an event on FED 0")
        self.setAppParam('skipNbEvents','unsignedInt','1','FEROL',0)
        time.sleep(7)
        self.checkAppState("SyncLoss","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        self.checkAppParam('nbEventsMissingData','unsignedLong',0,operator.eq,"BU")
        dumps = self.getFiles("dump_run"+str(self.runNumber).zfill(6)+"_event[0-9]+_fed[0-9]+.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()
        time.sleep(1)

        self.startEvB()
        print("Duplicate an event on FED 0")
        self.setAppParam('duplicateNbEvents','unsignedInt','1','FEROL',0)
        time.sleep(7)
        self.checkAppState("SyncLoss","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        self.checkAppParam('nbEventsMissingData','unsignedLong',0,operator.eq,"BU")
        dumps = self.getFiles("dump_run"+str(self.runNumber).zfill(6)+"_event[0-9]+_fed[0-9]+.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0'),
             ('maxTimeWithIncompleteEvents','unsignedInt','0')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id,[
                ('fedSize','unsignedInt',self.fedSizes[id])
                ]) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0'),
             ('maxTimeWithIncompleteEvents','unsignedInt','0')
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
