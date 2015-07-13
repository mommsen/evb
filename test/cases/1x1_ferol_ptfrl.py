from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x1_ferol_ptfrl(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(2048)
        self.checkBU(2048)
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ])
        self._config.add( FEROL(symbolMap,evm,512) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
