import time

from TestCase import *
from Configuration import RU,BU


class case_2x2_cloud(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.setAppParam('rawDataDir','string',buDir,'BU',instance)
            self.setAppParam('metaDataDir','string',buDir,'BU',instance)
            self.setAppParam('hltParameterSetURL','string','file://'+buDir,'BU',instance)
        self.prepareAppliance(testDir+"/BU0",runNumber,activeResources=16,cloud=16)
        self.prepareAppliance(testDir+"/BU1",runNumber,activeResources=0,cloud=32)
        self.configureEvB()
        try:
            self.enableEvB(sleepTime=2,runNumber=runNumber)
        except StateException:
            self.checkAppState("Mist","BU",0)
            self.checkAppState("Cloud","BU",1)
        else:
            raise StateException("EvB should not be Enabled")
        time.sleep(5)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624,instance=0)
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
