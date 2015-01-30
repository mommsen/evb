import operator
import time

from TestCase import *
from Configuration import FEROL,RU,BU


class case_2x1_ferol_cycleStates(TestCase):

    def checkIt(self):
        self.checkEVM(8192)
        self.checkRU(8192)
        self.checkBU(16384)


    def runTest(self):
        self.configureEvB()
        self.enableEvB(runNumber=1,sleepTime=2)
        self.checkIt()
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=2,sleepTime=2)
        self.checkIt()
        self.stopEvB()
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=3,sleepTime=2)
        self.checkIt()
        self.stopEvB()
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=4,sleepTime=2)
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=5,sleepTime=2)
        self.checkIt()
        print("Fail the EVM")
        self.sendStateCmd('Fail','Failed','EVM')
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=6,sleepTime=2)
        self.checkIt()
        print("Fail the RU")
        self.sendStateCmd('Fail','Failed','RU')
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=7,sleepTime=2)
        self.checkIt()
        print("Fail the BU")
        self.sendStateCmd('Fail','Failed','BU')
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=8,sleepTime=2)
        self.checkIt()
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evmFedIds = range(4)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ],evmFedIds)
        for id in evmFedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )

        ruFedIds = range(4,8)
        ru = RU(symbolMap,[
             ('inputSource','string','FEROL')
            ],ruFedIds)
        for id in ruFedIds:
            self._config.add( FEROL(symbolMap,ru,[
                ('fedId','unsignedInt',str(id))
                ]) )

        self._config.add( evm )
        self._config.add( ru )

        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
