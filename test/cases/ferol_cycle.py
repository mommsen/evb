import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_ferol_cycle(TestCase):

    def runTest(self):
        #self.configureEvB()
        while True:
            self.configureEvB()
            self.enableEvB(sleepTime=2)
            #raw_input("Running, press Enter to halt...")
            #self.stopEvB()
            #raw_input("Stopped, press Enter to halt...")
            #self.halt('FEROL')
            #self.checkAppState('Halted','DummyFEROL')
            #raw_input("FEROLs halted, press Enter to halt EvB...")
            #self.halt('EVM')
            #self.halt('RU')
            #self.halt('BU')
            #self.checkState('Halted')
            self.haltEvB()
            #raw_input("Halted, press Enter to configure...")


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('dropAtSocket','boolean','false'),
             ('dropInputData','boolean','false'),
             ('numberOfResponders','unsignedInt','4')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('dropAtSocket','boolean','false'),
             ('dropInputData','boolean','false'),
             ('numberOfResponders','unsignedInt','4')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
