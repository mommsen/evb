import operator
import time

from TestCase import *
from Context import FEROL,RU,BU


class case_3x1_noFragments(TestCase):

    def runTest(self):
        enabledFerol = [0] + range(4,7)
        testDir="/tmp/evb_test/ramdisk"
        runNumber=time.strftime("%s",time.localtime())
        runDir=testDir+"/run"+runNumber
        self.prepareAppliance(testDir,runNumber)
        self.setAppParam('rawDataDir','string',testDir,'BU')
        self.setAppParam('metaDataDir','string',testDir,'BU')
        self.configureEvB()
        self.setAppParam('hltParameterSetURL','string','file://'+testDir,'BU')
        print("Enable EvB with run number "+str(runNumber))
        self.setParam("runNumber","unsignedInt",runNumber)
        self.enable('EVM')
        self.enable('RU')
        self.enable('BU')
        for id in enabledFerol:
            self.enable('FEROL',id)

        # Wait for ls timeouts
        time.sleep(10)

        sys.stdout.write("Stopping EvB")
        sys.stdout.flush()
        self.stop('EVM')
        try:
            self.waitForAppState('Ready','EVM',maxTries=5)
        except StateException:
            self.checkAppState('Draining','EVM')
        else:
            raise StateException("EvB should not be Ready")

        for id in enabledFerol:
            self.sendStateCmd('Stop','Draining','FEROL',id)
        self.clear('EVM')
        self.clear('RU')
        self.clear('BU')
        self.checkBuDir(testDir,runNumber,eventSize=24576)

        runNumber=time.strftime("%s",time.localtime())
        self.enableEvB(runNumber=runNumber)
        self.checkEVM(2048)
        self.checkRU(6144)
        self.checkBU(14336)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evm = RU(symbolMap,[
             ('inputSource','string','Socket'),
             ('fakeLumiSectionDuration','unsignedInt','5')
            ])
        for id in range(0,1):
            self._config.add( FEROL(symbolMap,evm,id) )

        ru1 = RU(symbolMap,[
             ('inputSource','string','Socket'),
            ])
        for id in range(1,4):
            self._config.add( FEROL(symbolMap,ru1,id) )

        ru2 = RU(symbolMap,[
             ('inputSource','string','Socket'),
            ])
        for id in range(4,7):
            self._config.add( FEROL(symbolMap,ru2,id) )

        self._config.add( evm )
        self._config.add( ru1 )
        self._config.add( ru2 )

        self._config.add( BU(symbolMap,[
             ('lumiSectionTimeout','unsignedInt','6'),
             ('staleResourceTime','unsignedInt','0')
            ]) )
