from TestCase import TestCase
from Context import RU,BU


class case_2x1_largeEvents(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(16384)
        self.checkRU(196608)
        self.checkBU(212992)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams((512,),16384)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(1,13),16384)
            ) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
