import time

from TestCase import *
from Context import RU,BU


class case_2x2_stopRequests(TestCase):

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
        self.enableEvB(runNumber=runNumber)
        self.writeResourceSummary(testDir+"/BU1",runNumber,stopRequested=True)
        time.sleep(15)
        self.checkAppState("Enabled","BU",0)
        self.checkAppState("Stopped","BU",1)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624,instance=0)
        self.stopEvB()
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.checkBuDir(buDir,runNumber,eventSize=26648,buInstance=instance)

        runNumber=time.strftime("%s",time.localtime())
        try:
            self.enableEvB(sleepTime=2,runNumber=runNumber)
        except StateException:
            self.checkAppState("Stopped","BU",1)
        else:
            raise StateException("EvB should not be Enabled")
        time.sleep(15)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624,instance=0)
        self.stopEvB()
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.checkBuDir(buDir,runNumber,eventSize=26648,buInstance=instance)

        runNumber=time.strftime("%s",time.localtime())
        self.writeResourceSummary(testDir+"/BU1",runNumber,stopRequested=False)
        self.enableEvB(runNumber=runNumber)
        self.checkAppState("Enabled","BU")
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        self.stopEvB()
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.checkBuDir(buDir,runNumber,eventSize=26648,buInstance=instance)


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','4')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
            ('lumiSectionTimeout','unsignedInt','6'),
            ('resourcesPerCore','double','1'),
            ('staleResourceTime','unsignedInt','0')
            ]) )
        self._config.add( BU(symbolMap,[
            ('lumiSectionTimeout','unsignedInt','6'),
            ('resourcesPerCore','double','1'),
            ('staleResourceTime','unsignedInt','0')
            ]) )
