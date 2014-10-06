import os
import re
import socket
import sys

class SymbolMap:

    def __init__(self,testCaseDir):

        try:
            self._symbolMapfile = testCaseDir + os.environ["EVB_SYMBOL_MAP"]
        except KeyError:
            print("Please specify the symbol map file to be used in the environment veriable EVB_SYMBOL_MAP")
            sys.exit(2)

        self._hostname = socket.gethostbyaddr(socket.gethostname())[0]
        self._map = {}

        hostTypeRegEx = re.compile('^([A-Za-z0-9_]+)SOAP_HOST_NAME')
        hostCount = 0

        try:
            with open(self._symbolMapfile) as symbolMapFile:
                for line in symbolMapFile:
                    if line.rstrip(): #skip empty lines
                        (key,val) = line.split()
                        if val == 'localhost':
                            val = self._hostname
                        self._map[key] = val
                        try:
                            hostType = hostTypeRegEx.findall(key)[0]
                            self._map[hostType + 'LAUNCHER_PORT'] = str(int(self._map['LAUNCHER_BASE_PORT']) + hostCount)
                            self._map[hostType + 'SOAP_PORT']     = str(int(self._map['SOAP_BASE_PORT'])     + hostCount)
                            self._map[hostType + 'I2O_PORT']      = str(int(self._map['I2O_BASE_PORT'])      + hostCount)
                            self._map[hostType + 'FRL_PORT']      = str(int(self._map['FRL_BASE_PORT'])      + hostCount)
                            hostCount += 1
                        except IndexError:
                            pass

        except EnvironmentError as e:
            print("Could not open "+self._symbolMapfile+": "+str(e))
            sys.exit(2)


    def getHostInfo(self,hostType):
        return {'launcherPort':self._map[hostType + '_LAUNCHER_PORT'],
                'soapHostname':self._map[hostType + '_SOAP_HOST_NAME'],
                'soapPort':self._map[hostType + '_SOAP_PORT'],
                'i2oHostname':self._map[hostType + '_I2O_HOST_NAME'],
                'i2oPort':self._map[hostType + '_I2O_PORT']}


    def parse(self,string):
        for (key,val) in self._map.iteritems():
            string = string.replace(key,val)
        return string
