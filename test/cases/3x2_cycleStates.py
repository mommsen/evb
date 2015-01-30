import time

from TestCase import TestCase
from Configuration import RU,BU


class case_3x2_cycleStates(TestCase):

    def checkIt(self):
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(51200)


    def runTest(self):
        self.configureEvB()
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=1,sleepTime=2)
        self.checkIt()
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=2,sleepTime=2)
        self.checkIt()
        self.stopEvB()
        self.enableEvB(runNumber=3,sleepTime=2)
        self.checkIt()
        print("Fail the EVM")
        self.sendStateCmd('Fail','Failed','EVM')
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=4,sleepTime=2)
        self.checkIt()
        print("Fail RU 0")
        self.sendStateCmd('Fail','Failed','RU',0)
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=5,sleepTime=2)
        self.checkIt()
        print("Fail BU 1")
        self.sendStateCmd('Fail','Failed','BU',1)
        self.haltEvB()
        time.sleep(1)
        self.configureEvB()
        self.enableEvB(runNumber=6,sleepTime=2)
        self.checkIt()
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ],(512,)) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
