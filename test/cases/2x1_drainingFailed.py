import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_drainingFailed(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)


    def runTest(self):
        self.configureEvB()
        self.enableEvB(runNumber=1)
        self.checkIt()

        print("Stop sending from FED 2")
        self.sendStateCmd('Halt','Halted','FEROL',2)
        time.sleep(5)
        self.checkEVM(0,0)
        self.halt('FEROL')
        self.checkAppState('Halted','FEROL')
        sys.stdout.write("Stopping EVM")
        sys.stdout.flush()
        self.stop('EVM')
        try:
            self.waitForAppState('Ready','EVM')
        except StateException:
            self.checkAppState('Draining','EVM')
        else:
            raise StateException("EvB should not be Ready")
        print("Recovering draining failure")
        self.haltEvB()
        self.configureEvB()
        self.enableEvB(runNumber=2)
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
