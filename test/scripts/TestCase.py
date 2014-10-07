import getopt
import operator
import os
import re
import socket
import sys
from time import sleep

import messengers
import Configuration


class StateException(Exception):
    pass

class ValueException(Exception):
    pass


class TestCase:

    def __init__(self,symbolMap):

        self._config = Configuration.Configuration()
        self.fillConfiguration(symbolMap)


    def __del__(self):
        for context in self._config.contexts:
            try:
                print("Stopping XDAQ on "+context.hostInfo['soapHostname']+":"+str(context.hostInfo['launcherPort']))
                messengers.sendCmdToLauncher("STOPXDAQ",context.hostInfo['soapHostname'],context.hostInfo['launcherPort'],context.hostInfo['soapPort'])
            except socket.error:
                pass


    def startXDAQs(self):
        try:
            for context in self._config.contexts:
                print("Starting XDAQ on "+context.hostInfo['soapHostname']+":"+str(context.hostInfo['launcherPort']))
                messengers.sendCmdToLauncher("STARTXDAQ",context.hostInfo['soapHostname'],context.hostInfo['launcherPort'],context.hostInfo['soapPort'])
        except socket.error:
            print("Cannot contact launcher")
            sys.exit(2)

        for context in self._config.contexts:
            sys.stdout.write("Checking "+context.hostInfo['soapHostname']+":"+context.hostInfo['soapPort']+" is listening")
            for x in xrange(10):
                if messengers.webPing(context.hostInfo['soapHostname'],context.hostInfo['soapPort']):
                    print(" okay")
                    break
                else:
                    sys.stdout.write('.')
                    sys.stdout.flush()
                    sleep(1)
            else:
                print(" NOPE")
                sys.exit(2)


    def createConfigureCmd(self):
        #contextRegEx = re.compile('<xc:Context url="http://(?P<hosttype>[A-Za-z0-9_]+)SOAP_HOST_NAME:')
        #applicationRegEx = re.compile('<xc:Application.*class="(?P<class>[A-Za-z:]+)".*instance="(?P<instance>[0-9]+)"')
        cmd = """<xdaq:Configure xmlns:xdaq=\"urn:xdaq-soap:3.0\">"""
        cmd += self._config.getPartition()
        cmd += "</xdaq:Configure>"
        return cmd


    def sendCmdToExecutive(self):
        urn = "urn:xdaq-application:lid=0"
        cmd = self.createConfigureCmd()
        print(cmd)
        for context in self._config.contexts:
            print("Configuring executive on "+context.hostInfo['soapHostname']+":"+context.hostInfo['soapPort'])
            response = messengers.sendSoapMessage(context.hostInfo['soapHostname'],context.hostInfo['soapPort'],urn,cmd);
            xdaqResponse = response.getElementsByTagName('xdaq:ConfigureResponse')
            if len(xdaqResponse) != 1:
                raise(messengers.SOAPexception("Did not get a proper configure response from "+
                                                context.hostInfo['soapHostname']+":"+context.hostInfo['soapPort']+":\n"+
                                                response.toprettyxml()))

    def sendStateCmd(self,cmd,newState,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    state = messengers.sendCmdToApp(command=cmd,**application)
                    if state != newState:
                        if state == 'Failed':
                            raise(StateException(app+":"+application['instance']+" has failed"))
                        else:
                            raise(StateException(app+":"+application['instance']+" is in state "+state+
                                                 " instead of target state "+newState))


        except KeyError:
            pass


    def changeState(self,cmd,newState):
        for app in self._config.applications.keys():
            self.sendStateCmd(cmd,newState,app)


    def checkAppState(self,targetState,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    state = messengers.getStateName(**application)
                    if state != targetState:
                        raise(StateException(app+":"+str(application['instance'])+" has not reached "+targetState+" state"))
        except KeyError:
            pass


    def waitForAppState(self,targetState,app,instance=None,maxTries=5):
        tries = 0
        while True:
            try:
                self.checkAppState(targetState,app,instance)
                break
            except StateException as e:
                tries += 1
                sys.stdout.write('.')
                sys.stdout.flush()
                if tries > maxTries:
                    raise(e)
                sleep(1)


    def waitForState(self,targetState,maxTries=5):
         for app in self._config.applications.keys():
            self.waitForAppState(targetState,app,maxTries=maxTries)


    def checkState(self,targetState):
        for app in self._config.applications.keys():
            self.checkAppState(targetState,app)


    def setAppParam(self,paramName,paramType,paramValue,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    messengers.setParam(paramName,paramType,paramValue,**application)
        except KeyError:
            pass


    def setParam(self,paramName,paramType,paramValue):
        for app in self._config.applications.keys():
            self.setAppParam(paramName,paramType,paramValue,app)


    def checkAppParam(self,paramName,paramType,expectedValue,comp,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    value = int(messengers.getParam(paramName,paramType,**application))
                    if comp(value,expectedValue):
                        print(app+":"+str(application['instance'])+" "+paramName+": "+str(value))
                    else:
                        raise(ValueException(paramName+" on "+app+":"+str(application['instance'])+
                                             " is "+str(value)+" instead of "+str(expectedValue)))
        except KeyError:
            pass


    def configure(self,app,instance=None):
        self.sendStateCmd('Configure','Configuring',app,instance)


    def enable(self,app,instance=None):
        self.sendStateCmd('Enable','Enabled',app,instance)


    def stop(self,app,instance=None):
        self.sendStateCmd('Stop','Draining',app,instance)


    def halt(self,app,instance=None):
        self.sendStateCmd('Halt','Halted',app,instance)


    def startPt(self):
        print("Starting pt")
        print self._config.pt
        for application in self._config.pt:
            messengers.sendCmdToApp(command='Configure',**application)
        for application in self._config.pt:
            messengers.sendCmdToApp(command='Enable',**application)


    def configureEvB(self):
        sys.stdout.write("Configuring EvB")
        sys.stdout.flush()
        self.changeState('Configure','Configuring')
        self.waitForState('Ready')
        print(" done")


    def enableEvB(self,sleepTime=5,runNumber=1):
        print("Enable EvB with run number "+str(runNumber))
        self.setParam("runNumber","unsignedInt",runNumber)
        self.enable('evb::EVM')
        self.enable('evb::RU')
        self.enable('evb::BU')
        self.enable('evb::test::DummyFEROL')
        self.checkState('Enabled')
        sys.stdout.write("Building for "+str(sleepTime)+"s...")
        sys.stdout.flush()
        sleep(sleepTime)
        print("done")
        self.checkState('Enabled')


    def stopEvB(self):
        sys.stdout.write("Stopping EvB")
        sys.stdout.flush()
        self.stop('evb::test::DummyFEROL')
        self.waitForAppState('Ready','evb::test::DummyFEROL',maxTries=15)
        self.stop('evb::EVM')
        self.waitForAppState('Ready','evb::EVM',maxTries=20)
        self.stop('evb::RU')
        self.stop('evb::BU')
        self.waitForState('Ready')
        print(" done")


    def haltEvB(self):
        print("Halt EvB")
        self.halt('evb::test::DummyFEROL')
        self.halt('evb::EVM')
        self.halt('evb::RU')
        self.halt('evb::BU')
        self.checkState('Halted')


    def checkEVM(self,superFragmentSize):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"evb::EVM")
        self.checkAppParam("eventCount","unsignedLong",1000,operator.gt,"evb::EVM")
        self.checkAppParam("eventRate","unsignedInt",1000,operator.gt,"evb::EVM")


    def checkRU(self,superFragmentSize,instance=None):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"evb::RU",instance)


    def checkBU(self,eventSize,instance=None):
        self.checkAppParam("eventSize","unsignedInt",eventSize,operator.eq,"evb::BU",instance)
        self.checkAppParam("nbEventsBuilt","unsignedLong",1000,operator.gt,"evb::BU",instance)
        self.checkAppParam("eventRate","unsignedInt",1000,operator.gt,"evb::BU",instance)


    def run(self):
        self.startXDAQs()
        self.sendCmdToExecutive()
        self.startPt()
        self.runTest()
