import time

from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x1_ferol_socket(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        #self.checkEVM(2048)
        #self.checkBU(2048)
        self.stopEvB()
        self.haltEvB()
        #time.sleep(300)


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('dropAtSocket','boolean','true')
            ])
        self._config.add( FEROL(symbolMap,evm,512) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
