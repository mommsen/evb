import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_2x1_syncLoss(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)


    def causeSyncLoss(self,app):
        if app == "EVM":
            fed=2
        else:
            fed=7
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
                self.checkAppState("SyncLoss",app)
                print(" done")
                break
            except StateException:
                pass
        if tries == 30:
            print(" FAILED")
            raise(StateException("EVM not in expected stats 'SyncLoss'"))
        time.sleep(2)
        if app == "EVM":
            self.checkAppState("Enabled","RU")
        else:
            self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","BU")
        self.checkAppParam('eventRate','unsignedInt',0,operator.eq,"EVM")
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed[0-9]+.txt$",app="EVM")
        if len(dumps) != 1:
            raise ValueException("Expected one FED dump file, but found: "+str(dumps))


    def runTest(self):
        self.configureEvB()
        self.enableEvB(runNumber=1)
        self.checkIt()

        self.sendResync()
        self.checkAppState("Enabled","EVM")
        self.checkAppState("Enabled","RU")
        self.checkAppState("Enabled","BU")

        self.causeSyncLoss("EVM")
        self.haltEvB()
        time.sleep(1)

        self.configureEvB()
        self.enableEvB(runNumber=2)
        self.checkIt()
        self.causeSyncLoss("RU")
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','0')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('checkCRC','unsignedInt','0')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
