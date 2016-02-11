import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_2x1_drainingFailed(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)

    def blockFed(self,id):
        sys.stdout.write("Stop sending from FED "+str(id))
        sys.stdout.flush()
        self.sendStateCmd('Stop','Draining','FEROL',id)
        self.waitForAppState('Ready','FEROL',id)
        print("done")
        time.sleep(5)
        self.checkEVM(0,0)
        sys.stdout.write("Stopping EVM")
        sys.stdout.flush()
        self.stop('EVM')
        try:
            self.waitForAppState('Ready','EVM',maxTries=10)
        except StateException:
            self.checkAppState('Draining','EVM')
        else:
            raise StateException("EvB should not be Ready")
        print("Recovering draining failure")
        for i in range(0,8):
            if id != i:
                self.sendStateCmd('Stop','Draining','FEROL',i)
        self.clearEvB()


    def runTest(self):
        self.configureEvB()

        for fed in range(0,8):
            self.enableEvB(runNumber=fed+1)
            self.checkIt()
            self.blockFed(fed)

        self.enableEvB(runNumber=9)
        self.checkIt()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
