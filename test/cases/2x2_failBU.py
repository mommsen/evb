import time

from TestCase import *
from Configuration import RU,BU


class case_2x2_failBU(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir)
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.setAppParam('rawDataDir','string',buDir,'BU',instance)
            self.setAppParam('metaDataDir','string',buDir,'BU',instance)
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        self.enableEvB(sleepTime=12,runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        print("Fail BU0")
        self.sendStateCmd('Fail','Failed','BU',0)
        self.checkBuDir(testDir,runNumber,eventSize=28672,buInstance=0)
        try:
            self.stopEvB()
        except StateException:
            print(" failed")
        else:
            raise StateException("EvB should not drain when a BU failed")
        self.haltEvB()
        self.checkBuDir(testDir,runNumber,eventSize=28672,buInstance=1)
        self.configureEvB()
        runNumber=time.strftime("%s",time.localtime())
        self.enableEvB(runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(24576)
        self.checkBU(26624)
        self.stopEvB()


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ],(512,)) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local')
            ]) )
        self._config.add( BU(symbolMap,[]) )
        self._config.add( BU(symbolMap,[]) )
