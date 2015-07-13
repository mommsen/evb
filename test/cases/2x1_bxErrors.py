import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_bxErrors(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB(sleepTime=1)
        print("1 BX error on FED 1")
        self.setAppParam('nbBXerrors','unsignedInt','1','FEROL',1)
        time.sleep(2)
        print("3 BX error on FED 5")
        self.setAppParam('nbBXerrors','unsignedInt','3','FEROL',5)
        time.sleep(2)
        print("2 BX error on FED 7")
        self.setAppParam('nbBXerrors','unsignedInt','2','FEROL',7)
        time.sleep(3)
        self.checkState("Enabled")
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)
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
             ('lumiSectionTimeout','unsignedInt','0'),
             ('checkCRC','unsignedInt','0')
            ]) )
