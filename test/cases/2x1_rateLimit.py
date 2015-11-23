import operator
import time

from TestCase import TestCase
from Context import RU,BU


class case_2x1_rateLimit(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB(sleepTime=7)
        self.checkEVM(2048,95)
        self.checkAppParam("eventRate","unsignedInt",105,operator.lt,"EVM")
        self.checkRU(24576)
        self.checkBU(26624,90)
        print("Setting trigger rate to 1000 Hz")
        self.setAppParam("maxTriggerRate","unsignedInt",1000,"EVM")
        time.sleep(5)
        self.checkEVM(2048,990)
        self.checkAppParam("eventRate","unsignedInt",1010,operator.lt,"EVM")
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('maxTriggerRate','unsignedInt',100)
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true'),
             ('lumiSectionTimeout','unsignedInt','0')
            ]) )
