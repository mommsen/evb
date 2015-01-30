from TestCase import TestCase
from Configuration import FEROL,RU,BU


class case_3x1_scal(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(9232)
        self.checkRU(22832,1)
        self.checkRU(4080,2)
        self.checkBU(36144)
        self.stopEvB()
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evmFedIds = range(4)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('fakeLumiSectionDuration','unsignedInt','23'),
             ('dummyScalFedSize','unsignedInt','1024'),
             ('scalFedId','unsignedInt','997')
            ],evmFedIds)
        for id in evmFedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )

        ruFedIds = range(4,8)
        ru = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('dummyScalFedSize','unsignedInt','14624'),
             ('scalFedId','unsignedInt','998')
            ],ruFedIds)
        for id in ruFedIds:
            self._config.add( FEROL(symbolMap,ru,[
                ('fedId','unsignedInt',str(id))
                ]) )

        self._config.add( evm )
        self._config.add( ru )
        self._config.add( RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('dummyScalFedSize','unsignedInt','4064'),
             ('scalFedId','unsignedInt','999')
            ],()) )
        self._config.add( BU(symbolMap,[
             ('dropEventData','boolean','true')
            ]) )
