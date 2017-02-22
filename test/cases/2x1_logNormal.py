import operator

from TestCase import TestCase
from Context import RU,BU


class case_2x1_logNormal(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkAppParam("eventRate","unsignedInt",1000,operator.gt,"EVM")
        self.checkAppParam("nbEventsBuilt","unsignedLong",1000,operator.gt,"BU")
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),
             ('dummyFedSizeMin','unsignedInt','512'),
             ('dummyFedSizeMax','unsignedInt','2048'))
             + self.getFedParams((512,),2048,1024)
            ) )
        self._config.add( RU(symbolMap,(
             ('inputSource','string','Local'),)
             + self.getFedParams(range(1,13),2048,1024)
            ) )
        self._config.add( BU(symbolMap,(
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            )) )
