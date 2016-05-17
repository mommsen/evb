import time

from TestCase import *
from Context import RU,BU


class case_2x2_bandwidth(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.setAppParam('rawDataDir','string',buDir,'BU',instance)
            self.setAppParam('metaDataDir','string',buDir,'BU',instance)
            self.setAppParam('hltParameterSetURL','string','file://'+buDir,'BU',instance)
        self.prepareAppliance(testDir+"/BU0",runNumber)
        self.prepareAppliance(testDir+"/BU1",runNumber)
        self.configureEvB()
        self.enableEvB()
        self.writeResourceSummary(testDir+"/BU0",runNumber,outputBandwidthMB=110)
        self.writeResourceSummary(testDir+"/BU1",runNumber,outputBandwidthMB=130)
        time.sleep(5)
        self.checkAppState("Throttled","BU",0)
        self.checkAppState("Blocked","BU",1)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624,instance=0)
        self.writeResourceSummary(testDir+"/BU0",runNumber,outputBandwidthMB=90)
        self.writeResourceSummary(testDir+"/BU1",runNumber,outputBandwidthMB=110)
        time.sleep(5)
        self.checkAppState("Enabled","BU",0)
        self.checkAppState("Throttled","BU",1)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        self.stopEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
            ('resourcesPerCore','double','1'),
            ('staleResourceTime','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
            ('resourcesPerCore','double','1'),
            ('staleResourceTime','unsignedInt','0')
            ]) )
