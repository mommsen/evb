import operator
import os

from TestCase import TestCase
from Context import FEROL,RU,BU


class case_1x2_playback(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkAppParam("eventRate","unsignedInt",500,operator.gt,"EVM")
        self.checkAppParam("nbEventsBuilt","unsignedLong",1000,operator.gt,"BU")
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket')
            ])
        self._config.add( FEROL(symbolMap,evm,841,[
            ('usePlayback','boolean','true'),
            ('playbackDataFile','string',os.environ["EVB_TESTER_HOME"]+'/cases/csc_00307511_EmuRUI01_Local_000.raw')
            ]) )
        self._config.add( FEROL(symbolMap,evm,843,[
            ('usePlayback','boolean','true'),
            ('playbackDataFile','string',os.environ["EVB_TESTER_HOME"]+'/cases/csc_00307511_EmuRUI03_Local_000.raw')
            ]) )
        self._config.add( evm )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
