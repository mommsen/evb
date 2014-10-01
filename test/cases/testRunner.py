#!/usr/bin/python

import getopt
import httplib
import os
import re
import socket
import sys


soapTemplate="""
<soap-env:Envelope
  soap-env:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
  xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/">
  <soap-env:Header/>
  <soap-env:Body>
  %s
  </soap-env:Body>
</soap-env:Envelope>
"""


class SymbolMap:

    def __init__(self):

        try:
            self._symbolMapfile = os.environ["TESTS_SYMBOL_MAP"]
        except KeyError:
            print("Please specify the symbol map file to be used in the environment veriable TESTS_SYMBOL_MAP")
            sys.exit(2)

        try:
            self._testerHome = os.environ["EVB_TESTER_HOME"]
        except KeyError:
            print("Please specify the test directory to be used in the environment veriable EVB_TESTER_HOME")
            sys.exit(2)

        if not os.path.isdir(self._testerHome):
            print(self._testerHome + " is not a directory")
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
            print("Could not open " + self._symbolMapfile + ": " + str(e))
            sys.exit(2)



    def createConfigureCommand(self,testName):

        try:
            testDir = self._testerHome + "/tmp/" + testName
            try:
                os.makedirs(testDir)
            except OSError:
                if not os.path.isdir(testDir):
                    raise

            launchers = []
            contextRegEx = re.compile('<xc:Context url="http://([A-Za-z0-9_]+)SOAP_HOST_NAME:([A-Za-z0-9_]+)SOAP_PORT')
            config = "<xdaq:Configure xmlns:xdaq=\"urn:xdaq-soap:3.0\">\n"
            try:
                with open(self._testerHome + "/cases/" + testName + "/configuration.template.xml") as template:
                    for line in template:
                        config += line
                        context = contextRegEx.search(line)
                        if context:
                            hostType = context.group(1)
                            launchers.append((self._map[hostType + 'SOAP_HOST_NAME'],
                                              int(self._map[hostType + 'LAUNCHER_PORT']),
                                              "STARTXDAQ"+self._map[hostType + 'SOAP_PORT']))
                for (key,val) in self._map.iteritems():
                    config = config.replace(key,val)
                config += "</xdaq:Configure>"

            except EnvironmentError as e:
                print("Could not open config file for test " + testName + ": " + str(e))
                sys.exit(2)


            with open(testDir + "/configureCommand.xml","w") as conf:
                conf.write(soapTemplate%(config))

        except EnvironmentError as e:
            print("Could not create configure command file in " + testDir + ": " + str(e))
            sys.exit(2)

        return launchers


class TestCase:

    def __init__(self,symbolMap,testName=None):

        if testName is None:
            print("Please specify a test name")
            sys.exit(2)

        self._testName = testName

        launchers = symbolMap.createConfigureCommand(testName)
        for l in launchers:
            print("Launching XDAQ on "+l[0]+":"+str(l[1]))
            self.sendCmdToLauncher(*l)


    def sendCmdToLauncher(self,host,port,cmd):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host,port))
        s.send(cmd)


    def sendSimpleCommand(self,app,instance,command):
        urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
        soapMessage = soapTemplate%("<xdaq:"+command+" xmlns:xdaq=\"urn:xdaq-soap:3.0\"/>")
        headers = {"Content-Type":"text/xml",
                   "Content-Description":"SOAP Message",
                   "SOAPAction":urn}
        server = httplib.HTTPConnection(self._host,self._port)
        #server.set_debuglevel(4)
        server.request("POST","",soapMessage,headers)
        response = server.getresponse().read()
        xmldoc = minidom.parseString(response)
        xdaqResponse = xmldoc.getElementsByTagName('xdaq:'+command+'Response')
        if len(xdaqResponse) == 0:
            xdaqResponse = xmldoc.getElementsByTagName('xdaq:'+command.lower()+'Response')
        if len(xdaqResponse) != 1:
            raise(SOAPexception("Did not get a proper FSM response from "+app+":"+str(instance)+":\n"+xmldoc.toprettyxml()))
        try:
            newState = xdaqResponse[0].firstChild.attributes['xdaq:stateName'].value
        except (AttributeError,KeyError):
            raise(SOAPexception("FSM response from "+app+":"+str(instance)+" is missing state name:\n"+xmldoc.toprettyxml()))
        return newState


    def getParam(self,app,instance,paramName,paramType):
        urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
        paramGet = """<xdaq:ParameterGet xmlns:xdaq=\"urn:xdaq-soap:3.0\"><p:properties xmlns:p="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct"><p:%(paramName)s xsi:type="xsd:%(paramType)s"/></p:properties></xdaq:ParameterGet>""" % {'paramName':paramName,'paramType':paramType,'app':app}
        soapMessage = self._soapTemplate%(paramGet)
        headers = {"Content-Type":"text/xml",
                   "Content-Description":"SOAP Message",
                   "SOAPAction":urn}
        server = httplib.HTTPConnection(self._host,self._port)
        server.request("POST","",soapMessage,headers)
        response = server.getresponse().read()
        xmldoc = minidom.parseString(response)
        xdaqResponse = xmldoc.getElementsByTagName('p:'+paramName)
        if len(xdaqResponse) != 1:
            raise(SOAPexception("ParameterGetResponse response from "+app+":"+str(instance)+" is missing "+paramName+":\n"+xmldoc.toprettyxml()))
        return xdaqResponse[0].firstChild.data


    def getStateName(self,app,instance):
        return self.getParam("stateName","string",app,instance)



def usage():
    print """
testRunner.py --test testCase
          -h --help:         print this message and exit
          -t --test:         name of the test case
    """


def main(argv):

    testName = None

    try:
        opts,args = getopt.getopt(argv,"t:",["test="])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            usage()
            sys.exit()
        elif opt in ("-t", "--test"):
            testName = arg

    symbolMap = SymbolMap()
    testCase = TestCase(symbolMap,testName)
    print "test done"


if __name__ == "__main__":
    main(sys.argv[1:])
