import time

from TestCase import *
from Context import RU,BU


class case_2x2_returnResources(TestCase):

    def runTest(self):
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        for instance in range(2):
            buDir = testDir+"/BU"+str(instance)
            self.prepareAppliance(buDir,runNumber)
            self.setAppParam('rawDataDir','string',buDir,'BU',instance)
            self.setAppParam('metaDataDir','string',buDir,'BU',instance)
            self.setAppParam('hltParameterSetURL','string','file://'+buDir,'BU',instance)
        self.configureEvB()
        self.enableEvB(sleepTime=10,runNumber=runNumber)
        self.checkEVM(2048,eventRate=50)
        self.checkRU(24576)
        self.checkBU(26624,50,0)
        self.checkBU(0,0,1)
        print("Fail BU1")
        self.sendStateCmd('Fail','Failed','BU',1)
        time.sleep(5)
        self.checkBuDir(testDir+"/BU1",runNumber,eventSize=26648,buInstance=1)
        nbEventsInBU1 = self.getAppParam('nbEventsInBU','unsignedInt','BU',1)['BU1']
        time.sleep(5)
        self.checkAppParam('nbEventsInBU','unsignedInt',nbEventsInBU1,operator.eq,'BU',1)
        #raw_input("Press enter to continue")
        self.haltEvB()
        self.checkBuDir(testDir+"/BU0",runNumber,eventSize=26648,buInstance=0)
        #raw_input("Press enter to continue")


    def fillConfiguration(self,symbolMap):
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',(512,)),
             ('fakeLumiSectionDuration','unsignedInt','5'),
             ('maxTriggerRate','unsignedInt','100')
            ]) )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','Local'),
             ('fedSourceIds','unsignedInt',range(1,13))
            ]) )
        self._config.add( BU(symbolMap,[
             ('staleResourceTime','unsignedInt','0')
             ]) )
        self._config.add( BU(symbolMap,[
             ('staleResourceTime','unsignedInt','0'),
            ('minPriority','unsignedInt','3')
             ]) )
