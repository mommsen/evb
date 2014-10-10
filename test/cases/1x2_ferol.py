from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x2_ferol(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(16384)
        self.checkBU(16384)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        fedIds = range(1,9)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('fedSourceIds','unsignedInt',fedIds)
            ])
        for id in fedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
