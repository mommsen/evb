from TestCase import TestCase
from Configuration import RU,BU


class case_2x1_multiFEDs(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(12288)
        self.checkRU(24576)
        self.checkBU(36864)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ],(12,55,512,689,789,800)) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ],(3,9,65,78,124,154,255,854,957,1022,1111,1227)) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
