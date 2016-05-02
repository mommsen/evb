import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_2x1_corruptedEvents(TestCase):

    def checkIt(self):
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',0,operator.eq,"BU")


    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        self.prepareAppliance(testDir,runNumber=1)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(runNumber=1)
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()

        sys.stdout.write("Corrupt an event on FED 2 - tolerating")
        sys.stdout.flush()
        self.setAppParam('corruptNbEvents','unsignedInt','1','FEROL',2)
        self.waitForAppState("MissingData","EVM")
        print(" done")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkEVM(6144)
        self.checkRU(8192)
        self.checkBU(14336)
        self.checkIt()
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0002.txt$",app="EVM")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 2, but found: "+str(dumps))

        sys.stdout.write("Corrupt 2 events on FED 7 - not tolerating")
        sys.stdout.flush()
        self.setAppParam('corruptNbEvents','unsignedInt','2','FEROL',7)
        self.waitForAppState("Failed","RU")
        print(" done")
        self.checkAppState("MissingData","EVM")
        self.checkAppState("Enabled","BU")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0007.txt$",app="EVM")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 7, but found: "+str(dumps))

        self.haltEvB()
        self.checkBuDir(testDir,"000001")

        self.configureEvB()
        self.enableEvB(runNumber=2)
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()
        sys.stdout.write("Corrupt an event on FED 1 - tolerating")
        sys.stdout.flush()
        self.setAppParam('corruptNbEvents','unsignedInt','1','FEROL',1)
        self.waitForAppState("MissingData","EVM")
        print(" done")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkEVM(6144)
        self.checkRU(8192)
        self.checkBU(14336)
        self.checkIt()
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0002.txt$",app="EVM")
        if len(dumps) != 1:
            raise ValueException("Expected one dump file from FED 2, but found: "+str(dumps))
        self.sendResync()
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
        self.checkIt()
        self.stopEvB()
        self.checkBuDir(testDir,"000002")


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','0'),
             ('tolerateCorruptedEvents','boolean','true'),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','0'),
             ('tolerateCorruptedEvents','boolean','false')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('checkCRC','unsignedInt','1'),
             ('staleResourceTime','unsignedInt','0'),
             ('lumiSectionTimeout','unsignedInt','6'),
             ('numberOfBuilders','unsignedInt','2')
            ]) )
