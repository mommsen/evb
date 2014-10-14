import operator

from TestCase import TestCase
from Configuration import RU,BU


class case_2x1_logNormal(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkAppParam("eventRate","unsignedInt",1000,operator.gt,"EVM")
        self.checkAppParam("nbEventsBuilt","unsignedLong",1000,operator.gt,"BU")
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('dummyFedSize','unsignedInt','2048'),
             ('dummyFedSizeStdDev','unsignedInt','1024'),
             ('dummyFedSizeMin','unsignedInt','512'),
             ('dummyFedSizeMax','unsignedInt','2048'),
             ('useLogNormal','boolean','true')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('dummyFedSize','unsignedInt','2048'),
             ('dummyFedSizeStdDev','unsignedInt','1024'),
             ('useLogNormal','boolean','true')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
