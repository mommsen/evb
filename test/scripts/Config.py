import sys

import Configuration
from TestCase import TestCase

class Config(TestCase):

    def __init__(self,symbolMap,configFile,stdout):
        self._origStdout = sys.stdout
        sys.stdout = stdout
        self._config = Configuration.ConfigFromFile(symbolMap,configFile)


    def __del__(self):
        sys.stdout.flush()
        sys.stdout = self._origStdout
