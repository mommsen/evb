import operator

from TestCase import TestCase
from Context import FEROL,RU,BU

class case_2x1_RUmetaData(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(6144)
        self.checkRU(8336)
        self.checkBU(14480)
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',0,operator.eq,"BU")
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in (512,25,17):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        for id in range(1,5)+[1022,]:
            self._config.add( FEROL(symbolMap,ru,id) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
