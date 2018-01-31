import operator

from TestCase import TestCase
from Context import RU,BU

class case_2x1_metaData(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(6296)
        self.checkRU(24576)
        self.checkBU(30872)
        self.checkAppParam('nbCorruptedEvents','unsignedLong',0,operator.eq,"BU")
        self.checkAppParam('nbEventsWithCRCerrors','unsignedLong',0,operator.eq,"BU")
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,25,17)),
             ('createSoftFed1022','boolean','true'),
             ('maskedDipTopics','string','dip/CMS/DCS/CMS_HCAL/CMS_HCAL_CASTOR/state,dip/CMS/DCS/CMS_HCAL/CMS_HCAL_ZDC/state')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
