import operator

from TestCase import TestCase
from Context import FEROL,RU,BU


class case_2x1_ferol_logNormal(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkAppParam("eventRate","unsignedInt",100,operator.gt,"EVM")
        self.checkAppParam("nbEventsBuilt","unsignedLong",100,operator.gt,"BU")
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in range(0,4):
            self._config.add( FEROL(symbolMap,evm,id,[
                ('fedSize','unsignedInt','2048'),
                ('fedSizeStdDev','unsignedInt','1024')
                ]) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in range(4,8):
            self._config.add( FEROL(symbolMap,ru,id,[
                ('fedSize','unsignedInt','2048'),
                ('fedSizeStdDev','unsignedInt','1024')
                ]) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
