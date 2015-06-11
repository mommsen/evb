import operator

from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x1_ferol_rateLimit(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(2048,490)
        self.checkAppParam("eventRate","unsignedInt",510,operator.lt,"EVM")
        self.checkBU(2048,490)
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        self._config.add( FEROL(symbolMap,evm,512,[
             ('maxTriggerRate','unsignedInt',500)
             ]) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
