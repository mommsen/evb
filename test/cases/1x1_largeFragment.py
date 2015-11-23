from time import sleep
from TestCase import TestCase
from Context import FEROL,RU,BU


class case_1x1_largeFragment(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(22944)
        self.checkBU(22944)
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('numberOfResponders','unsignedInt','2')
            ])
        self._config.add( FEROL(symbolMap,evm,605,[
            ('fedSize','unsignedInt','1632')
            ]) )
        self._config.add( FEROL(symbolMap,evm,607,[
            ('fedSize','unsignedInt','416')
            ]) )
        self._config.add( FEROL(symbolMap,evm,608,[
            ('fedSize','unsignedInt','416')
            ]) )
        self._config.add( FEROL(symbolMap,evm,609,[
            ('fedSize','unsignedInt','20480')
            ]) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0'),
             ('eventsPerRequest','unsignedInt','32')
            ]) )
