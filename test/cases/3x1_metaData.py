import operator

from TestCase import TestCase
from Context import RU,BU

class case_3x1_metaData(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(6144)
        self.checkRU(24576,1)
        self.checkRU(144,2)
        self.checkBU(30864)
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',0,operator.eq,"BU")
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,25,17))
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(1022,))
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
