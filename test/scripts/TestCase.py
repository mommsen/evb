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
        try:
            shutil.rmtree("/tmp/evb_test")
        except OSError:
            pass
        for context in self._config.contexts:
            try:
                print("Stopping XDAQ on "+context.apps['soapHostname']+":"+str(context.apps['launcherPort']))
                print(messengers.sendCmdToLauncher("stopXDAQ",context.apps['soapHostname'],context.apps['launcherPort'],context.apps['soapPort']))
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
                print(messengers.sendCmdToLauncher("startXDAQ",context.apps['soapHostname'],context.apps['launcherPort'],context.apps['soapPort']))
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


    def checkAppState(self,targetState,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or instance == application['instance']:
                    state = messengers.getStateName(**application)
                    if state != targetState:
                        raise(StateException(app+str(application['instance'])+" is not in expected state '"+targetState+"', but '"+state+"'"))
        except KeyError:
            pass


    def waitForAppState(self,targetState,app,instance=None,maxTries=10):
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
                    print("")
                    raise(e)
                sleep(1)


    def waitForState(self,targetState,maxTries=10):
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
                    tries = 0
                    while tries < 3:
                        tries += 1
                        value = messengers.getParam(paramName,paramType,**application)
                        if comp(value,expectedValue):
                            print(app+str(application['instance'])+" "+paramName+": "+str(value))
                            return

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


    def clear(self,app,instance=None):
        self.sendStateCmd('Clear','Configuring',app,instance)


    def halt(self,app,instance=None):
        self.sendStateCmd('Halt','Halted',app,instance)


    def startPt(self):
        print("Starting pt")
        for application in self._config.ptUtcp:
            messengers.sendCmdToApp(command='Configure',**application)
        for application in self._config.ptUtcp:
            messengers.sendCmdToApp(command='Enable',**application)


    def resetPtFrl(self):
        print("Resetting pt::frl")
        for application in self._config.ptFrl:
            messengers.sendCmdToApp(command='Reset',**application)


    def configureEvB(self):
        sys.stdout.write("Configuring EvB")
        sys.stdout.flush()
        self.configure('FEROL')
        self.waitForAppState('Ready','FEROL')
        self.configure('EVM')
        self.configure('RU')
        self.configure('BU')
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


    def stopEvB(self,runDir=None):
        eventToStop = self.getEventInFuture()
        self.setAppParam('stopAtEvent','unsignedInt',eventToStop,'FEROL')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'EVM')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'RU')
        sys.stdout.write("Stopping EvB at event "+str(eventToStop))
        sys.stdout.flush()

        self.stop('FEROL')
        if runDir:
            for rawFile in glob.glob(runDir+"/*.raw"):
                os.remove(rawFile)
            self.waitForAppState('Ready','FEROL',maxTries=60)

        self.stop('EVM')
        if runDir:
            for rawFile in glob.glob(runDir+"/*.raw"):
                os.remove(rawFile)
        self.waitForAppState('Ready','EVM',maxTries=60)
        self.stop('RU')
        self.stop('BU')
        self.waitForState('Ready')
        print(" done")
        self.checkEventCount()


    def clearEvB(self):
        sys.stdout.write("Clearing EvB")
        sys.stdout.flush()
        self.clear('EVM')
        self.clear('RU')
        self.clear('BU')
        self.waitForState('Ready')
        print(" done")


    def haltEvB(self):
        print("Halt EvB")
        self.halt('FEROL')
        self.halt('EVM')
        self.halt('RU')
        self.halt('BU')
        self.checkState('Halted')
        self.resetPtFrl();


    def sendResync(self):
        resyncAtEvent = self.getEventInFuture()
        self.setAppParam('resyncAtEvent','unsignedInt',resyncAtEvent,'FEROL')
        sys.stdout.write("Resync at event "+str(resyncAtEvent))
        sys.stdout.flush()
        lastEvent = self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0']
        tries = 0
        while self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0'] >= lastEvent and tries < 30:
            # a resync will reset the event number to 1
            sleep(1)
            tries += 1
            sys.stdout.write(".")
            sys.stdout.flush()
        if tries == 30:
            print(" FAILED")
            raise(StateException("No resync seen"))
        else:
            sleep(2)
            print(" done")


    def getEventInFuture(self):
        eventRate = self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        sleep(2)
        eventRate += self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        sleep(2)
        eventRate += self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        lastEvent = self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0']
        return lastEvent + max(2*eventRate,10000)



    def checkEVM(self,superFragmentSize,eventRate=1000):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"EVM")
        self.checkAppParam("eventCount","unsignedLong",500,operator.ge,"EVM")
        self.checkAppParam("eventRate","unsignedInt",eventRate,operator.ge,"EVM")


    def checkRU(self,superFragmentSize,instance=None):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"RU",instance)


    def checkBU(self,eventSize,eventRate=1000,instance=None):
        self.checkAppParam("eventSize","unsignedInt",eventSize,operator.eq,"BU",instance)
        self.checkAppParam("nbEventsBuilt","unsignedLong",500,operator.ge,"BU",instance)
        self.checkAppParam("eventRate","unsignedInt",eventRate,operator.ge,"BU",instance)


    def checkEventCount(self,allBuilt=True):
        sleep(1) # assure counters are up-to-date
        evmEventCount = self.getAppParam('eventCount','unsignedLong','EVM')['EVM0']
        evmEventsBuilt = self.getAppParam('nbEventsBuilt','unsignedLong','EVM')['EVM0']
        buEventsBuilt = self.getAppParam('nbEventsBuilt','unsignedLong','BU')
        buEventCount = reduce(lambda x,y:x+y,buEventsBuilt.values())
        if evmEventsBuilt != buEventCount:
            raise ValueError("EVM claims "+str(evmEventsBuilt)+" events were built, while BU count gives "+str(buEventCount)+" events")
        if allBuilt and evmEventCount != buEventCount:
            raise ValueError("EVM counted "+str(evmEventCount)+" events, while BUs built "+str(buEventCount)+" events")


    def prepareAppliance(self,testDir,runNumber,activeResources=160,quarantinedResources=0,staleResources=0,activeRunCMSSWMaxLS=-1,activeRunNumQueuedLS=-1,cloud=0):
        try:
            shutil.rmtree(testDir)
        except OSError:
            pass
        os.makedirs(testDir+'/appliance')
        stat = os.statvfs(testDir)
        ramDiskOccupancy = 1 - stat.f_bfree/float(stat.f_blocks)
        with open(testDir+'/appliance/resource_summary','w') as resourceSummary:
            resourceSummary.write('{"ramdisk_occupancy": '+str(ramDiskOccupancy)+
                ', "broken": 0, "idle": 16, "used":'+str(activeResources-16)+
                ', "activeFURun": '+str(runNumber)+
                ', "active_resources": '+str(activeResources)+
                ', "quarantined": '+str(quarantinedResources)+
                ', "stale_resources": '+str(staleResources)+
                ', "activeRunCMSSWMaxLS": '+str(activeRunCMSSWMaxLS)+
                ', "activeRunNumQueuedLS": '+str(activeRunNumQueuedLS)+
                ', "cloud": '+str(cloud)+'}')
        with open(testDir+'/HltConfig.py','w') as config:
            config.write("dummy HLT menu for EvB test")
        with open(testDir+'/fffParameters.jsn','w') as param:
            param.write('{')
            param.write('    "SCRAM_ARCH": "slc6_amd64_gcc491",')
            param.write('    "CMSSW_VERSION": "CMSSW_7_4_2",')
            param.write('    "TRANSFER_MODE": "TIER0_TRANSFER_OFF"')
            param.write('}')
        return ramDiskOccupancy

    def checkBuDir(self,testDir,runNumber,eventSize=None,buInstance=None):
        runDir=testDir+"/run"+runNumber
        print( subprocess.Popen(["ls","--full-time","-rt","-R",runDir], stdout=subprocess.PIPE).communicate()[0] )
        if os.path.isdir(runDir+"/open"):
            raise FileException(runDir+"/open still exists")
        for jsdFile in ('rawData.jsd','EoLS.jsd','EoR.jsd'):
            jsdPath = runDir+"/jsd/"+jsdFile
            if not os.path.isfile(jsdPath):
                raise FileException(jsdPath+" does not exist")
        for hltFile in ('HltConfig.py','fffParameters.jsn'):
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
            totalEvents = eolsData[0] + eolsData[3]
            if buInstance is None and totalEvents != eolsData[2]:
                raise ValueException("Total event count from EVM "+str(eolsData[2])+" does not match "+str(totalEvents)+" in "+eolsFile)

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
