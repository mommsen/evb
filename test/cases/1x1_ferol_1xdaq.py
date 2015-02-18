import time

from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_1x1_ferol_1xdaq(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(2048)
        self.checkBU(2048)
        self.setAppParam('writeNextFragmentsToFile','unsignedInt','4','EVM')
        time.sleep(1)
        dumps = self.getFiles("dump_run000001_event[0-9]+_fed0512.txt$")
        if len(dumps) != 4:
            raise ValueException("Expected 4 dump file from FED 512, but found: "+str(dumps))
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        bu = BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ])
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ],(bu,))
        self._config.add( FEROL(symbolMap,evm,512) )
        self._config.add( evm )
