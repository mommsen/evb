import getopt
import glob
import json
import filecmp
import operator
import os
import re
import shutil
import socket
import subprocess
import sys
from time import sleep

import messengers
import Configuration


class LauncherException(Exception):
    pass

class StateException(Exception):
    pass

class ValueException(Exception):
    pass

class FileException(Exception):
    pass


class TestCase:

    def __init__(self,symbolMap,stdout):

        self._origStdout = sys.stdout
        sys.stdout = stdout
        self._config = Configuration.Configuration()
        self.fillConfiguration(symbolMap)
        self._configCmd = self.createConfigureCmd()
        print("**************************************")
        print(self._configCmd)
        print("**************************************")



    def __del__(self):
        for context in self._config.contexts:
            try:
                print("Stopping XDAQ on "+context.apps['soapHostname']+":"+str(context.apps['launcherPort']))
                messengers.sendCmdToLauncher("stopXDAQ",context.apps['soapHostname'],context.apps['launcherPort'],context.apps['soapPort'])
            except socket.error:
                pass
        for file in glob.glob("/tmp/dump_*txt"):
            os.remove(file)
        sys.stdout.flush()
        sys.stdout = self._origStdout


    def startXDAQs(self):
        try:
            for context in self._config.contexts:
                print("Starting XDAQ on "+context.apps['soapHostname']+":"+str(context.apps['launcherPort']))
                messengers.sendCmdToLauncher("startXDAQ",context.apps['soapHostname'],context.apps['launcherPort'],context.apps['soapPort'])
        except socket.error:
            raise LauncherException("Cannot contact launcher")

        for context in self._config.contexts:
            sys.stdout.write("Checking "+context.apps['soapHostname']+":"+context.apps['soapPort']+" is listening")
            for x in xrange(10):
                if messengers.webPing(context.apps['soapHostname'],context.apps['soapPort']):
                    print(" okay")
                    break
                else:
                    sys.stdout.write('.')
                    sys.stdout.flush()
                    sleep(1)
            else:
                print(" NOPE")
                raise LauncherException(context.apps['soapHostname']+":"+context.apps['soapPort']+" is not listening")


    def createConfigureCmd(self):
        cmd = """<xdaq:Configure xmlns:xdaq=\"urn:xdaq-soap:3.0\">"""
        cmd += self._config.getPartition()
        cmd += "</xdaq:Configure>"
        return cmd


    def sendCmdToExecutive(self):
        urn = "urn:xdaq-application:lid=0"
        for context in self._config.contexts:
            print("Configuring executive on "+context.apps['soapHostname']+":"+context.apps['soapPort'])
            response = messengers.sendSoapMessage(context.apps['soapHostname'],context.apps['soapPort'],urn,self._configCmd);
            xdaqResponse = response.getElementsByTagName('xdaq:ConfigureResponse')
            if len(xdaqResponse) != 1:
                raise(messengers.SOAPexception("Did not get a proper configure response from "+
                                                context.apps['soapHostname']+":"+context.apps['soapPort']+":\n"+
                                                response.toprettyxml()))

    def sendStateCmd(self,cmd,newState,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    state = messengers.sendCmdToApp(command=cmd,**application)
                    if state != newState:
                        if state == 'Failed':
                            raise(StateException(app+str(application['instance'])+" has failed"))
                        else:
                            raise(StateException(app+str(application['instance'])+" is in state "+state+
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
                        raise(StateException(app+str(application['instance'])+" is not in expected state '"+targetState+"', but '"+state+"'"))
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


    def getAppParam(self,paramName,paramType,app,instance=None):
        params= {}
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    params[app+str(application['instance'])] = messengers.getParam(paramName,paramType,**application)
        except KeyError:
            pass
        return params


    def getParam(self,paramName,paramType):
        params = {}
        for app in self._config.applications.keys():
            params.update( self.getAppParam(paramName,paramType,app) )


    def checkAppParam(self,paramName,paramType,expectedValue,comp,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    value = messengers.getParam(paramName,paramType,**application)
                    if comp(value,expectedValue):
                        print(app+str(application['instance'])+" "+paramName+": "+str(value))
                    else:
                        raise(ValueException(paramName+" on "+app+str(application['instance'])+
                                             " is "+str(value)+" instead of "+str(expectedValue)))
        except KeyError:
            pass


    def waitForAppParam(self,paramName,paramType,expectedValue,comp,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    value = None
                    while not comp(value,expectedValue) and not sleep(1):
                        value = messengers.getParam(paramName,paramType,**application)

        except KeyError:
            pass


    def getFiles(self,regex,dir="/tmp"):
        return [f for f in os.listdir(dir) if re.search(regex,f)]


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
        self.enable('EVM')
        self.enable('RU')
        self.enable('BU')
        self.enable('FEROL')
        self.checkState('Enabled')
        if sleepTime > 0:
            sys.stdout.write("Building for "+str(sleepTime)+"s...")
            sys.stdout.flush()
            sleep(sleepTime)
            print("done")
            self.checkState('Enabled')


    def stopEvB(self):
        eventRate = self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        lastEvent = self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0']
        eventToStop = lastEvent + max(4*eventRate,1000)
        sys.stdout.write("Stopping EvB at event "+str(eventToStop))
        sys.stdout.flush()
        self.setAppParam('stopAtEvent','unsignedInt',eventToStop,'FEROL')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'EVM')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'RU')

        self.stop('FEROL')
        self.waitForAppState('Ready','FEROL',maxTries=15)
        self.stop('EVM')
        self.waitForAppState('Ready','EVM',maxTries=30)
        self.stop('RU')
        self.stop('BU')
        self.waitForState('Ready')
        print(" done")
        self.checkEventCount()


    def haltEvB(self):
        print("Halt EvB")
        self.halt('FEROL')
        self.halt('EVM')
        self.halt('RU')
        self.halt('BU')
        self.checkState('Halted')


    def checkEVM(self,superFragmentSize):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"EVM")
        self.checkAppParam("eventCount","unsignedLong",1000,operator.gt,"EVM")
        self.checkAppParam("eventRate","unsignedInt",500,operator.gt,"EVM")


    def checkRU(self,superFragmentSize,instance=None):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"RU",instance)


    def checkBU(self,eventSize,instance=None):
        self.checkAppParam("eventSize","unsignedInt",eventSize,operator.eq,"BU",instance)
        self.checkAppParam("nbEventsBuilt","unsignedLong",1000,operator.gt,"BU",instance)
        self.checkAppParam("eventRate","unsignedInt",500,operator.gt,"BU",instance)


    def checkEventCount(self):
        sleep(1) # assure counters are up-to-date
        evmEventCount = self.getAppParam('eventCount','unsignedLong','EVM')['EVM0']
        buEventCounts = self.getAppParam('nbEventsBuilt','unsignedLong','BU')
        buEventCount = reduce(lambda x,y:x+y,buEventCounts.values())
        if evmEventCount != buEventCount:
            raise ValueError("EVM counted "+str(evmEventCount)+" events, while BUs built "+str(buEventCount)+" events")


    def prepareAppliance(self,testDir):
        try:
            shutil.rmtree(testDir)
        except OSError:
            pass
        os.makedirs(testDir)
        with open(testDir+'/HltConfig.py','w') as config:
            config.write("dummy HLT menu for EvB test")
        with open(testDir+'/SCRAM_ARCH','w') as arch:
            arch.write("slc6_amd64_gcc472")
        with open(testDir+'/CMSSW_VERSION','w') as version:
            version.write("cmssw_noxdaq")
        stat = os.statvfs(testDir)
        return 1 - stat.f_bfree/float(stat.f_blocks)


    def checkBuDir(self,testDir,runNumber,eventSize=None,buInstance=None):
        if buInstance is not None:
            runDir=testDir+"/BU"+str(buInstance)+"/run"+runNumber
        else:
            runDir=testDir+"/run"+runNumber
        print( subprocess.Popen(["ls","--full-time","-rt","-R",runDir], stdout=subprocess.PIPE).communicate()[0] )
        if os.path.isdir(runDir+"/open"):
            raise FileException(runDir+"/open still exists")
        for jsdFile in ('rawData.jsd','EoLS.jsd','EoR.jsd'):
            jsdPath = runDir+"/jsd/"+jsdFile
            if not os.path.isfile(jsdPath):
                raise FileException(jsdPath+" does not exist")
        for hltFile in ('HltConfig.py','CMSSW_VERSION','SCRAM_ARCH'):
            hltPath = runDir+"/hlt/"+hltFile
            if not os.path.isfile(hltPath):
                raise FileException(hltPath+" does not exist")
            if not filecmp.cmp(hltPath,testDir+"/"+hltFile,False):
                raise FileException("Files "+testDir+"/"+hltFile+" and "+hltPath+" are not identical")

        runLsCounter = 0
        runFileCounter = 0
        runEventCounter = 0
        fileCounter = 0
        lastLumiWithFiles = 0
        lsRegex = re.compile('.*_ls([0-9]+)_EoLS.jsn')
        for eolsFile in sorted(glob.glob(runDir+"/*_EoLS.jsn")):
            lumiSection = lsRegex.match(eolsFile).group(1)
            with open(eolsFile) as file:
                eolsData = [int(f) for f in json.load(file)['data']]
            if buInstance is None and eolsData[0] != eolsData[2]:
                raise ValueException("Total event count "+str(eolsData[2])+" does not match "+str(eolsData[0])+" in "+eolsFile)

            eventCounter = 0
            if eolsData[1] > 0:
                runLsCounter += 1
                lastLumiWithFiles = int(lumiSection)
                jsonFiles = glob.glob(runDir+"/run"+runNumber+"_ls"+lumiSection+"_index*jsn")
                fileCounter = len(jsonFiles)
                runFileCounter += fileCounter
                for jsonFile in jsonFiles:
                    with open(jsonFile) as file:
                        jsonData = [int(f) for f in json.load(file)['data']]
                    eventCounter += jsonData[0]
                    if eventSize:
                        rawFile = jsonFile.replace('.jsn','.raw')
                        try:
                            fileSize = os.path.getsize(rawFile)
                        except OSError:
                            raise FileException(rawFile+" does not exist")
                            eventCount = fileSize/eventSize
                            if eventCount != jsonData[0]:
                                raise ValueException("expected "+str(jsonData[0])+", but found "+str(eventCount)+" events in JSON file "+jsonFile)

            else:
                fileCounter = len( glob.glob(runDir+"/run"+runNumber+"_ls"+lumiSection+"*[raw,jsn]") ) - 1

            if eventCounter != eolsData[0]:
                raise ValueException("expected "+str(eolsData[0])+" events in LS "+lumiSection+", but found "+str(eventCounter)+" events in raw data JSON files")
            if fileCounter != eolsData[1]:
                raise ValueException("expected "+str(eolsData[1])+" files for LS "+lumiSection+", but found "+str(fileCounter)+" raw data files")
            runEventCounter += eventCounter

        eorFile=runDir+"/run"+runNumber+"_ls0000_EoR.jsn"
        try:
            with open(eorFile) as file:
                eorData = [int(f) for f in json.load(file)['data']]
        except IOError:
            raise FileException("cannot locate the EoR json file "+eorFile)
        if runEventCounter != eorData[0]:
            raise ValueException("expected "+str(eorData[0])+" events in run "+runNumber+", but found "+str(runEventCounter)+" events in raw data JSON files")
        if runFileCounter != eorData[1]:
            raise ValueException("expected "+str(eorData[1])+" files for run "+runNumber+", but found "+str(runFileCounter)+" raw data files")
        if runLsCounter != eorData[2]:
            raise ValueException("expected "+str(eorData[2])+" lumi sections in run "+runNumber+", but found raw data files for "+str(runLsCounter)+" lumi sections")
        if lastLumiWithFiles != eorData[3]:
            raise ValueException("expected last lumi section "+str(eorData[3])+" for run "+runNumber+", but found raw data files up to lumi section "+str(lastLumiWithFiles))

        shutil.rmtree(runDir)


    def run(self):
        self.startXDAQs()
        self.sendCmdToExecutive()
        self.waitForState('Halted')
        self.startPt()
        self.runTest()
