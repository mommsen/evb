from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x2_ferol_ptfrl(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(16384)
        self.checkBU(16384)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ])
        for id in range(1,9):
            self._config.add( FEROL(symbolMap,evm,id) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
