import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_2x1_syncLossTolerating(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)


    def causeSyncLoss(self,fed):
        if fed in range(0,4):
            app = "EVM"
        else:
            app = "RU"
        resyncAtEvent = self.getEventInFuture()
        self.setAppParam('resyncAtEvent','unsignedInt',resyncAtEvent,'FEROL',fed)
        sys.stdout.write("Resync on FED "+str(fed)+" at event "+str(resyncAtEvent))
        sys.stdout.flush()
        tries = 0
        while tries < 60:
            time.sleep(1)
            tries += 1
            sys.stdout.write(".")
            sys.stdout.flush()
            try:
                self.checkAppState("MissingData",app)
                print(" done")
                break
            except StateException:
                pass
        if tries == 30:
            print(" FAILED")
            raise(StateException("EVM not in expected stats 'MissingData'"))
        time.sleep(2)
        if app == "EVM":
            self.checkAppState("Enabled","RU")
            self.checkEVM(6144)
            self.checkRU(8192)
        else:
            self.checkAppState("Enabled","EVM")
            self.checkEVM(8192)
            self.checkRU(6144)
        self.checkAppState("Enabled","BU")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed[0-9]+.txt$")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))
        time.sleep(10)
        self.checkAppState("Failed",app)


    def runTest(self):
        self.configureEvB()
        self.enableEvB(runNumber=1)
        self.checkIt()

        self.sendResync()
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")

        self.causeSyncLoss(2)
        self.haltEvB()
        time.sleep(1)

        self.configureEvB()
        self.enableEvB(runNumber=2)
        self.checkIt()
        self.causeSyncLoss(7)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0'),
             ('maxTimeWithIncompleteEvents','unsignedInt','5')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('tolerateOutOfSequenceEvents','boolean','true'),
             ('checkCRC','unsignedInt','0'),
             ('maxTimeWithIncompleteEvents','unsignedInt','5')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
