from TestCase import *
from Configuration import FEROL,RU


class case_2x1_dropInputData(TestCase):

    def runTest(self):
        self.configureEvB()
        self.enableEvB()
        self.checkEVM(8192)
        self.checkRU(8192)
        self.haltEvB()


    def fillConfiguration(self,symbolMap):
        evmFedIds = range(4)
        evm = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('fedSourceIds','unsignedInt',evmFedIds),
             ('dropInputData','boolean','true')
            ])
        for id in evmFedIds:
            self._config.add( FEROL(symbolMap,evm,[
                ('fedId','unsignedInt',str(id))
                ]) )

        ruFedIds = range(4,8)
        ru = RU(symbolMap,[
             ('inputSource','string','FEROL'),
             ('fedSourceIds','unsignedInt',ruFedIds),
             ('dropInputData','boolean','true')
            ])
        for id in ruFedIds:
            self._config.add( FEROL(symbolMap,ru,[
                ('fedId','unsignedInt',str(id))
                ]) )

        self._config.add( evm )
        self._config.add( ru )
