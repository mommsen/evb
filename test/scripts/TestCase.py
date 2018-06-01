import getopt
import glob
import json
import filecmp
import operator
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import time
import multiprocessing as mp

import messengers
import Configuration


class LauncherException(Exception):
    pass

class StateException(Exception):
    """
    exception which is thrown when an application
    is not in the expected state
    """
    pass

class FailedState(Exception):
    """
    exception which is thrown when an application
    is in failed state
    """
    pass

class ValueException(Exception):
    pass

class FileException(Exception):
    pass


import xmlrpclib

def startXDAQ(context,testname,logLevel):
    ret = "Starting XDAQ on "+context.hostinfo['soapHostname']+":"+str(context.hostinfo['launcherPort'])+"\n"

    server = xmlrpclib.ServerProxy("http://%s:%s" % (context.hostinfo['soapHostname'],context.hostinfo['launcherPort']))

    if logLevel is not None:
        server.setLogLevel(context.hostinfo['soapPort'],logLevel)

    ret += server.startXDAQ(context.hostinfo['soapPort'],testname)
    return ret


def stopXDAQ(context):
    ret = "Stopping XDAQ on "+context.hostinfo['soapHostname']+":"+str(context.hostinfo['launcherPort'])+"\n"

    server = xmlrpclib.ServerProxy("http://%s:%s" % (context.hostinfo['soapHostname'],context.hostinfo['launcherPort']))

    ret += server.stopXDAQ(context.hostinfo['soapPort'])
    return ret


def sendCmdToExecutive(context,configCmd):
    urn = "urn:xdaq-application:lid=0"
    ret = "Configuring executive on "+context.hostinfo['soapHostname']+":"+context.hostinfo['soapPort']
    response = messengers.sendSoapMessage(context.hostinfo['soapHostname'],context.hostinfo['soapPort'],
                                            urn,configCmd);
    try:
        xdaqResponse = response.getElementsByTagName('xdaq:ConfigureResponse')
        if len(xdaqResponse) != 1:
            raise AttributeError
    except AttributeError:
        raise(messengers.SOAPexception("Did not get a proper configure response from "+
                                        context.hostinfo['soapHostname']+":"+context.hostinfo['soapPort']+":\n"+
                                        response.toprettyxml()))
    return ret


def sendStateCmdToApp(cmd,newState,app):
    state = messengers.sendCmdToApp(command=cmd,**app)
    if state not in newState:
        if state == 'Failed':
            raise(StateException(app['class']+":"+app['instance']+" has failed"))
        else:
            raise(StateException(app['class']+":"+app['instance']+" is in state "+state+
                                    " instead of target state "+str(newState)))


# Work around hanging python pool when pressing ctrl-C
# http://noswap.com/blog/python-multiprocessing-keyboardinterrupt
def init_worker():
    signal.signal(signal.SIGINT, signal.SIG_IGN)


class TestCase:

    def __init__(self,config,stdout,afterStartupCallback = None):
        """
        @param afterStartupCallback is a callback which will be called
        after the EVB has been enabled when nbMeasurements is zero.
        If this is left None, the user is prompted to press enter
        to terminate the test. This function is called with the
        TestCase object as argument.
        """

        self._origStdout = sys.stdout
        sys.stdout = stdout
        self._config = config
        self._pool = mp.Pool(20,init_worker)
        self.afterStartupCallback = afterStartupCallback

        # standard multiprocessing pool does not work well with exceptions
        # over xmlrpc, so we use a pool based on threads (instead of procecesses)
        # for xmlrpc operations (we are mostly waiting for IO so this should
        # not be a problem for performance)
        from multiprocessing.pool import ThreadPool
        self._xmlrpcPool = ThreadPool(20)

        self._xdaqLogLevel = None


    def destroy(self):
        try:
            shutil.rmtree("/tmp/evb_test")
        except OSError:
            pass
        if self._config:
            self.stopXDAQs()
        self._pool.close()
        self._pool.join()
        sys.stdout.flush()
        sys.stdout = self._origStdout


    def setXdaqLogLevel(self, newLevel):
        self._xdaqLogLevel = newLevel

    def stopXDAQs(self):
        # stops all XDAQ processes we started before
        import logging
        logging.info("stopping all XDAQs")

        results = [self._xmlrpcPool.apply_async(stopXDAQ, args=(c,)) for c in self._config.contexts.values()]
        for r in results:
            print(r.get(timeout=30))
        
        logging.info("done stopping all XDAQs")


    def startXDAQs(self,testname):
        try:
            results = [self._xmlrpcPool.apply_async(startXDAQ, args=(c,testname, self._xdaqLogLevel)) for c in self._config.contexts.values()]
            for r in results:
                try:
                    print(r.get(timeout=30))
                except mp.TimeoutError:
                    print(r)
        except socket.error:
            raise LauncherException("Cannot contact launcher")

        for context in self._config.contexts.values():
            sys.stdout.write("Checking "+context.hostinfo['soapHostname']+":"+context.hostinfo['soapPort']+" is listening")
            for x in xrange(10):
                if messengers.webPing(context.hostinfo['soapHostname'],context.hostinfo['soapPort']):
                    print(" okay")
                    break
                else:
                    sys.stdout.write('.')
                    sys.stdout.flush()
                    time.sleep(1)
            else:
                print(" NOPE")
                raise LauncherException(context.hostinfo['soapHostname']+":"+context.hostinfo['soapPort']+" is not listening")


    def sendCmdToExecutive(self):
        results = [self._pool.apply_async(sendCmdToExecutive, args=(context,self._config.getConfigCmd(key))) for key,context in self._config.contexts.items()]
        for r in results:
            r.get(timeout=30)


    def sendStateCmd(self,cmd,newState,app,instance=None):
        try:
            if instance is not None:
                for application in self._config.applications[app]:
                    if str(instance) == application['instance']:
                        sendStateCmdToApp(cmd,newState,application)
            else:
                results = [self._pool.apply_async(sendStateCmdToApp, args=(cmd,newState,app)) for app in self._config.applications[app]]
                for r in results:
                    r.get(timeout=30)
        except KeyError:
            pass


    def checkAppState(self,targetState,app,instance=None):
        """
        Checks the state of all the applications of type 'app'.
        If at least one of them is not in the specified targetState
        or is in Failed state, an exception is raised.
        """
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    state = messengers.getStateName(**application)
                    if state not in targetState:
                        if state == 'Failed':
                            raise(FailedState(app+application['instance']+" has Failed"))
                        else:
                            raise(StateException(app+application['instance']+" is not in expected state '"+str(targetState)+"', but '"+state+"'"))
        except KeyError:
            pass


    def waitForAppState(self,targetState,app,instance=None,maxTries=30,runDir=None):
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
                    print(" failed")
                    raise(e)
                if runDir:
                    for rawFile in glob.glob(runDir+"/*.raw"):
                        os.remove(rawFile)
                time.sleep(1)


    def waitForState(self,targetState,maxTries=10):
         for app in self._config.applications.keys():
            self.waitForAppState(targetState,app,maxTries=maxTries)


    def checkState(self,targetState):
        """
        Checks the application state of all applications
        associated to this event builder.
        """
        for app in self._config.applications.keys():
            self.checkAppState(targetState,app)


    def setAppParam(self,paramName,paramType,paramValue,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    try:
                        messengers.setParam(paramName,paramType,paramValue,**application)
                    except messengers.SOAPexception as e:
                        if application['app'] in ('ferol::FerolController','ferol40::Ferol40Controller'):
                            pass
                        else:
                            raise e
        except KeyError:
            pass


    def setParam(self,paramName,paramType,paramValue):
        for app in self._config.applications.keys():
            self.setAppParam(paramName,paramType,paramValue,app)


    def getAppParam(self,paramName,paramType,app,instance=None):
        params= {}
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    params[app+application['instance']] = messengers.getParam(paramName,paramType,**application)
        except KeyError:
            pass
        return params


    def getParam(self,paramName,paramType):
        params = {}
        for app in self._config.applications.keys():
            params.update( self.getAppParam(paramName,paramType,app) )


    def checkParam(self,paramName,paramType,expectedValue,comp,app,application):
        tries = 0
        while tries < 3:
            value = messengers.getParam(paramName,paramType,**application)
            if comp(value,expectedValue):
                print(app+application['instance']+" "+paramName+": "+str(value))
                return
            time.sleep(1)
            tries += 1

        raise(ValueException(paramName+" on "+app+application['instance']+
                             " is "+str(value)+" instead of "+str(expectedValue)))


    def checkAppParam(self,paramName,paramType,expectedValue,comp,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    self.checkParam(paramName,paramType,expectedValue,comp,app,application)
        except KeyError:
            pass


    def waitForAppParam(self,paramName,paramType,expectedValue,comp,app,instance=None):
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    value = None
                    while not comp(value,expectedValue) and not time.sleep(1):
                        value = messengers.getParam(paramName,paramType,**application)

        except KeyError:
            pass


    def checkRate(self):
        print("Checking if rate is >10 Hz:")
        try:
            evm = self._config.applications["EVM"][0]
            dropAtSocket = ( messengers.getParam("dropAtSocket","boolean",**evm) == "true" )
            dropAtRU = ( messengers.getParam("dropInputData","boolean",**evm) == "true" )
            if not dropAtSocket:
                self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"EVM")
                self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"RU")
            if not dropAtRU and not dropAtSocket:
                self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"BU")
            return
        except ValueException as e:
            ruCounts = self.getAppParam("eventCount","unsignedLong","RU")
            str = ""
            for ru in sorted(ruCounts.keys()):
                if ruCounts[ru] == 0:
                    str += " "+ru
            if len(str):
                print("RUs with no events:"+str)
            else:
                buCounts = self.getAppParam("nbEventsBuilt","unsignedLong","BU")
                for bu in sorted(buCounts.keys()):
                    if buCounts[bu] == 0:
                        str += " "+bu
                if len(str):
                    print("BUs with no events:"+str)
            #raw_input("Press Enter to continue...")
            raise e


    def getFedParams(self,fedIds,dummyFedSize,dummyFedSizeDev=0):
        fedSourceIds = []
        ferolSources = []
        for id in fedIds:
            fedSourceIds.append(id)
            ferolSources.append( (
                ('fedId','unsignedInt',id),
                ('dummyFedSize','unsignedInt',str(dummyFedSize)),
                ('dummyFedSizeStdDev','unsignedInt',str(dummyFedSizeDev))
                ) )
        return ('fedSourceIds','unsignedInt',fedSourceIds),('ferolSources','Struct',ferolSources)


    def getFiles(self,regex,app,instance=None,dir="/tmp"):
        files = []
        try:
            for application in self._config.applications[app]:
                if instance is None or str(instance) == application['instance']:
                    server = xmlrpclib.ServerProxy("http://%s:%s" % (application['soapHostname'],application['launcherPort']))

                    files.extend( eval(server.getFiles(application['soapPort'],dir)) )
        except KeyError:
            pass
        return [f for f in files if re.search(regex,f)]


    def configure(self,app,instance=None):
        self.sendStateCmd('Configure','Configuring',app,instance)


    def enable(self,app,instance=None):
        self.sendStateCmd('Enable',('Enabled','Enabling'),app,instance)


    def stop(self,app,instance=None):
        self.sendStateCmd('Stop',('Draining','Stopping'),app,instance)


    def clear(self,app,instance=None):
        self.sendStateCmd('Clear','Configuring',app,instance)


    def halt(self,app,instance=None):
        self.sendStateCmd('Halt',('Halted','Halting'),app,instance)


    def startPt(self):
        print("Starting pt")
        for application in self._config.ptUtcp:
            messengers.sendCmdToApp(command='Configure',**application)
        for application in self._config.ptUtcp:
            messengers.sendCmdToApp(command='Enable',**application)
        for application in self._config.ptIBV:
            messengers.sendCmdToApp(command='connect',**application)
        for application in self._config.ptIBV:
            count = 0
            while messengers.getStateName(**application) != 'Enabled':
                time.sleep(1)
                count += 1
                if count > 120:
                    raise(StateException("Failed to connect pt::ibv for "+str(application)))


    def configureEvB(self,maxTries=10):
        sys.stdout.write("Configuring EvB")
        sys.stdout.flush()
        self.configure('FEROL')
        self.waitForAppState(('Ready','Configured'),'FEROL')
        self.configure('EVM')
        self.configure('RU')
        self.configure('BU')
        self.waitForState(('Ready','Configured'),maxTries)
        print(" done")


    def enableEvB(self,sleepTime=5,runNumber=1,maxTries=10):
        sys.stdout.write("Enabling EvB with run number "+str(runNumber)+"...")
        sys.stdout.flush()
        self.setParam("runNumber","unsignedInt",runNumber)
        self.enable('EVM')
        self.enable('RU')
        self.enable('BU')
        self.enable('FEROL')
        self.waitForState('Enabled',maxTries)
        print("done")
        if sleepTime > 0:
            sys.stdout.write("Building for "+str(sleepTime)+"s...")
            sys.stdout.flush()
            time.sleep(sleepTime)
            print("done")
            self.checkState('Enabled')


    def stopEvB(self,runDir=None,maxTries=10):
        eventToStop = self.getEventInFuture()
        self.setAppParam('stopAtEvent','unsignedInt',eventToStop,'FEROL')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'EVM')
        self.setAppParam('stopLocalInputAtEvent','unsignedInt',eventToStop,'RU')
        sys.stdout.write("Stopping EvB at event "+str(eventToStop))
        sys.stdout.flush()

        self.stop('FEROL')
        self.waitForAppState('Ready','FEROL',maxTries=60,runDir=runDir)

        self.stop('EVM')
        self.waitForAppState('Ready','EVM',maxTries=60,runDir=runDir)
        self.stop('RU')
        self.stop('BU')
        self.waitForState('Ready',maxTries)
        print(" done")
        self.checkEventCount()


    def clearEvB(self,maxTries=10):
        sys.stdout.write("Clearing EvB")
        sys.stdout.flush()
        self.clear('EVM')
        self.clear('RU')
        self.clear('BU')
        self.waitForState('Ready',maxTries)
        print(" done")


    def haltEvB(self,maxTries=10):
        sys.stdout.write("Halting EvB")
        sys.stdout.flush()
        self.halt('FEROL')
        self.waitForAppState('Halted','FEROL')
        self.halt('EVM')
        self.halt('RU')
        self.halt('BU')
        self.waitForState('Halted',maxTries)
        print(" done")
        time.sleep(1)


    def sendResync(self):
        resyncAtEvent = self.getEventInFuture()
        self.setAppParam('resyncAtEvent','unsignedInt',resyncAtEvent,'FEROL')
        sys.stdout.write("Resync at event "+str(resyncAtEvent))
        sys.stdout.flush()
        lastEvent = self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0']
        tries = 0
        while self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0'] >= lastEvent and tries < 30:
            # a resync will reset the event number to 1
            time.sleep(1)
            tries += 1
            sys.stdout.write(".")
            sys.stdout.flush()
        if tries == 30:
            print(" FAILED")
            raise(StateException("No resync seen"))
        else:
            time.sleep(2)
            print(" done")


    def getEventInFuture(self):
        time.sleep(2)
        eventRate = self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        time.sleep(2)
        eventRate += self.getAppParam('eventRate','unsignedInt','EVM',0)['EVM0']
        try:
            # Use roughtly a median value of the FEROL event counts
            # In case of a tolerated syncLoss, some FEROLs will have huge counts
            ferolEventNumbers = sorted( self.getAppParam('lastEventNumber','unsignedInt','FEROL').values() )
            middleIndex = len(ferolEventNumbers) // 2
            return ferolEventNumbers[middleIndex] + eventRate
        except IndexError:
            return self.getAppParam('lastEventNumber','unsignedInt','EVM',0)['EVM0'] + 3*eventRate


    def checkEVM(self,superFragmentSize,eventRate=100):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"EVM")
        self.checkAppParam("eventCount","unsignedLong",500,operator.ge,"EVM")
        self.checkAppParam("eventRate","unsignedInt",eventRate,operator.ge,"EVM")


    def checkRU(self,superFragmentSize,instance=None):
        self.checkAppParam("superFragmentSize","unsignedInt",superFragmentSize,operator.eq,"RU",instance)


    def checkBU(self,eventSize,eventRate=100,instance=None):
        self.checkAppParam("eventSize","unsignedInt",eventSize,operator.eq,"BU",instance)
        self.checkAppParam("nbEventsBuilt","unsignedLong",5*eventRate,operator.ge,"BU",instance)
        self.checkAppParam("eventRate","unsignedInt",eventRate,operator.ge,"BU",instance)


    def checkEventCount(self,allBuilt=True):
        time.sleep(2) # assure counters are up-to-date
        evmEventCount = self.getAppParam('eventCount','unsignedLong','EVM')['EVM0']
        evmEventsBuilt = self.getAppParam('nbEventsBuilt','unsignedLong','EVM')['EVM0']
        buEventsBuilt = self.getAppParam('nbEventsBuilt','unsignedLong','BU')
        buEventCount = reduce(lambda x,y:x+y,buEventsBuilt.values())
        if evmEventsBuilt != buEventCount:
            raise ValueError("EVM claims "+str(evmEventsBuilt)+" events were built, while BU count gives "+str(buEventCount)+" events")
        if allBuilt and evmEventCount != buEventCount:
            raise ValueError("EVM counted "+str(evmEventCount)+" events, while BUs built "+str(buEventCount)+" events")


    def writeResourceSummary(self,testDir,runNumber,activeResources=160,quarantinedResources=0,staleResources=0,activeRunCMSSWMaxLS=-1,activeRunNumQueuedLS=-1,outputBandwidthMB=100,cloud=0,stopRequested=False):
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
                ', "bu_stop_requests_flag": '+str(stopRequested).lower()+
                ', "activeRunNumQueuedLS": '+str(activeRunNumQueuedLS)+
                ', "activeRunLSBWMB": '+str(outputBandwidthMB)+
                ', "cloud": '+str(cloud)+'}')
        return ramDiskOccupancy


    def prepareAppliance(self,testDir,runNumber):
        try:
            shutil.rmtree(testDir)
        except OSError:
            pass
        os.makedirs(testDir+'/appliance')
        with open(testDir+'/HltConfig.py','w') as config:
            config.write("dummy HLT menu for EvB test")
        with open(testDir+'/fffParameters.jsn','w') as param:
            param.write('{')
            param.write('    "SCRAM_ARCH": "slc6_amd64_gcc491",')
            param.write('    "CMSSW_VERSION": "CMSSW_7_4_2",')
            param.write('    "TRANSFER_MODE": "TIER0_TRANSFER_OFF"')
            param.write('}')
        return self.writeResourceSummary(testDir,runNumber)


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
            fileSizes = 0
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
                            raise ValueException("expected "+str(jsonData[0])+", but found "+str(eventCount)+" events in raw file "+rawFile)
                        if fileSize != jsonData[1]:
                            raise ValueException("expected a file size of "+str(jsonData[1])+" Bytes, but "+rawFile+" has a size of "+str(fileSize)+" Bytes")
                        fileSizes += fileSize

            else:
                fileCounter = len( glob.glob(runDir+"/run"+runNumber+"_ls"+lumiSection+"*[raw,jsn]") ) - 1

            if eventCounter != eolsData[0]:
                raise ValueException("expected "+str(eolsData[0])+" events in LS "+lumiSection+", but found "+str(eventCounter)+" events in raw data JSON files")
            if fileCounter != eolsData[1]:
                raise ValueException("expected "+str(eolsData[1])+" files for LS "+lumiSection+", but found "+str(fileCounter)+" raw data files")
            if eventSize and fileSizes != eolsData[4]:
                raise ValueException("expected "+str(eolsData[4])+" Bytes written for LS "+lumiSection+", but found "+str(fileSizes)+" Bytes")
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


    def getDataPoint(self):
        """
        retrieves super fragment size and event rate from the EVM
        and RU applications
        """
        sizes = self.getAppParam("superFragmentSize","unsignedInt","EVM")
        sizes.update( self.getAppParam("superFragmentSize","unsignedInt","RU") )
        sizes.update( self.getAppParam("eventSize","unsignedInt","BU") )
        rates = self.getAppParam("eventRate","unsignedInt","EVM")
        rates.update( self.getAppParam("eventRate","unsignedInt","RU") )
        rates.update( self.getAppParam("eventRate","unsignedInt","BU") )
        return {'rates':rates,'sizes':sizes}


    def getThroughputMB(self,dataPoint):
        """
        calculates the throughput in MByte/sec by summing over the superfragment
        sizes at all RUs and multiplying by the total event rate
        """
        try:
            size = sum(list(x['sizes']['RU0'] for x in dataPoint))/float(len(dataPoint))
            rate = sum(list(x['rates']['RU0'] for x in dataPoint))/float(len(dataPoint))
        except KeyError:
            size = sum(list(x['sizes']['RU1'] for x in dataPoint))/float(len(dataPoint))
            rate = sum(list(x['rates']['RU1'] for x in dataPoint))/float(len(dataPoint))
        return int(size*rate/1000000)


    def calculateFedSize(self,fedId,fragSize,fragSizeRMS):
        if fedId == self._config.evmFedId:
            return 1024,0
        else:
            return int(fragSize),int(fragSizeRMS)


    def setFragmentSizes(self,fragSize,fragSizeRMS):
        """
        configures the fragment size to be generated at the FEROL/FRL
        """

        try:
            for application in self._config.applications['FEROL']:
                for app in self._config.contexts[(application['soapHostname'],application['soapPort'])].applications:
                    if app.params['instance'] == application['instance']:
                        if app.params['class'] == 'ferol::FerolController':
                            for prop in app.properties:
                                for stream in range(2):
                                    if prop[0] == 'expectedFedId_'+str(stream):
                                        fedSize,fedSizeRMS = self.calculateFedSize(prop[2],fragSize,fragSizeRMS)
                                        messengers.setParam("Event_Length_bytes_FED"+str(stream),'unsignedInt',str(fedSize),**application)
                                        messengers.setParam("Event_Length_Stdev_bytes_FED"+str(stream),'unsignedInt',str(fedSizeRMS),**application)
                        elif app.params['class'] == 'ferol40::Ferol40Controller':
                            for prop in app.properties:
                                if prop[0] == 'InputPorts':
                                    ports = []
                                    for stream,item in enumerate(prop[2]):
                                    #for item in prop[2]:
                                        for p in item:
                                            if p[0] == 'expectedFedId':
                                                fedSize,fedSizeRMS = self.calculateFedSize(p[2],fragSize,fragSizeRMS)
                                                ports.append((('Event_Length_bytes','unsignedInt',str(fedSize)),('Event_Length_Stdev_bytes','unsignedInt',str(fedSizeRMS))))
                                                messengers.setParam("Event_Length_bytes_FED"+str(stream),"unsignedInt",str(fedSize),**application)
                                                messengers.setParam("Event_Length_Stdev_bytes_FED"+str(stream),"unsignedInt",str(fedSizeRMS),**application)
                                    #messengers.setParam('InputPorts','Array',ports,**application)
        except KeyError:
            for application in self._config.applications['RU']+self._config.applications['EVM']:
                for app in self._config.contexts[(application['soapHostname'],application['soapPort'])].applications:
                    if app.params['instance'] == application['instance'] and app.params['class'] in ('evb::EVM','evb::RU'):
                        for prop in app.properties:
                            if prop[0] == 'fedSourceIds':
                                sources = []
                                for item in prop[2]:
                                    fedSize,fedSizeRMS = self.calculateFedSize(item,fragSize,fragSizeRMS)
                                    sources.append((('dummyFedSize','unsignedInt',str(fedSize)),('dummyFedSizeStdDev','unsignedInt',str(fedSizeRMS))))
                                messengers.setParam('ferolSources','Array',sources,**application)


    def start(self):
        """
        configures and enables (starts) the event builder
        """

        runNumber=time.strftime("%s",time.localtime())
        self.configureEvB(maxTries=30)
        self.enableEvB(maxTries=30,runNumber=runNumber)
        self.checkRate()


    def prepare(self,testname,maxTries=10):
        self.startXDAQs(testname)
        self.sendCmdToExecutive()
        self.waitForState(('Halted','uninitialized','Initialized'),maxTries)
        self.startPt()


    def runScan(self,fragSize,fragSizeRMS,args):
        """
        runs a test for a single fragment size
        and writes out summary information. Tries
        multiple times in case of problems.
        """

        if not args['verbose']:
            self._origStdout.write("%dB:"%(fragSize))
            self._origStdout.flush()
        tries = 0
        while tries < 10:
            try:
                dataPoints = self.doIt(fragSize,fragSizeRMS,args)
                if not args['verbose']:
                    self._origStdout.write("%dMB/s "%(self.getThroughputMB(dataPoints)))
                    self._origStdout.flush()
                else:
                    print("%3.2f:%dMB/s" % (fragSize,self.getThroughputMB(dataPoints)))
                if args['long']:
                    time.sleep(60)
                return dataPoints
            except (FailedState,StateException,ValueException) as e:
                print(" failed: "+str(e))
                #raw_input("Press Enter to retry...")
                while tries < 10:
                    tries += 1
                    try:
                        self.haltEvB()
                        break
                    except FailedState as e:
                        print(" failed: "+str(e))
        raise e


    def doIt(self,fragSize,fragSizeRMS,args):
        """
        runs a test for a single fragment size
        and returns the obtained performance numbers
        """

        dataPoints = []
        self.setFragmentSizes(fragSize,fragSizeRMS)

        # start running
        self.start()

        if args['nbMeasurements'] == 0:
            dataPoints.append( self.getDataPoint() )

            if self.afterStartupCallback is None:
                # run until the user presses enter
                raw_input("Press Enter to stop...")
            else:
                self.afterStartupCallback(self)

        else:
            if args['long']:
                time.sleep(300)
            elif not args['quick']:
                time.sleep(60)
            for n in range(args['nbMeasurements']):
                if args['quick']:
                    time.sleep(1)
                else:
                    time.sleep(10)
                dataPoints.append( self.getDataPoint() )

        self.haltEvB()
        return dataPoints
