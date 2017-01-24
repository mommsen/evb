import sys
import time

import messengers
from TestCase import *


class ConfigCase(TestCase):

    defaultFedSize = 2048.

    def __init__(self,config,stdout,fedSizeScaleFactors):
        TestCase.__init__(self,config,stdout)
        self.fedSizeScaleFactors = fedSizeScaleFactors


    def __del__(self):
        TestCase.__del__(self)


    def checkRate(self,dropAtRU):
        print("Checking if rate is >10 Hz:")
        try:
            self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"EVM")
            self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"RU")
            if not dropAtRU:
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


    def getDataPoint(self):
        sizes = self.getAppParam("superFragmentSize","unsignedInt","EVM")
        sizes.update( self.getAppParam("superFragmentSize","unsignedInt","RU") )
        sizes.update( self.getAppParam("eventSize","unsignedInt","BU") )
        rates = self.getAppParam("eventRate","unsignedInt","EVM")
        rates.update( self.getAppParam("eventRate","unsignedInt","RU") )
        rates.update( self.getAppParam("eventRate","unsignedInt","BU") )
        return {'rates':rates,'sizes':sizes}


    def getThroughputMB(self,dataPoint):
        try:
            size = sum(list(x['sizes']['RU0'] for x in dataPoint))/float(len(dataPoint))
            rate = sum(list(x['rates']['RU0'] for x in dataPoint))/float(len(dataPoint))
        except KeyError:
            size = sum(list(x['sizes']['RU1'] for x in dataPoint))/float(len(dataPoint))
            rate = sum(list(x['rates']['RU1'] for x in dataPoint))/float(len(dataPoint))
        return int(size*rate/1000000)


    def sendsToEVM(self,application):
        for app in self._config.contexts[(application['soapHostname'],application['soapPort'])].applications:
            if app.params['class'] == 'ferol::FerolController' and app.params['instance'] == application['instance']:
                return app.params['sendsToEVM']
        return False


    def calculateFedSize(self,fedId,fragSize):
        (a,b,c,rms) = self.fedSizeScaleFactors[fedId]
        relSize = fragSize / ConfigCase.defaultFedSize
        fedSize = a + b*relSize + c*relSize*relSize
        fedSizeRMS = fedSize * rms
        return (int(fedSize+4)&~0x7,int(fedSizeRMS+4)&~0x7)


    def getFedSizes(self,fragSize,fragSizeRMS,application,isEVM):
        if isEVM:
            fedSizes = [1024,0]*2
        else:
            fedSizes = [fragSize,fragSizeRMS]*2

        if len(self.fedSizeScaleFactors):
            for app in self._config.contexts[(application['soapHostname'],application['soapPort'])].applications:
                if app.params['class'] == 'ferol::FerolController' and app.params['instance'] == application['instance']:
                    for prop in app.properties:
                        try:
                            if prop[0] == 'expectedFedId_0':
                                fedSizes[0:1] = self.calculateFedSize(prop[2],fragSize)
                            elif prop[0] == 'expectedFedId_1':
                                fedSizes[2:3] = self.calculateFedSize(prop[2],fragSize)
                        except KeyError:
                            if prop[2] != '0xffffffff':
                                print("Missing scale factor for FED id "+prop[2])
        return fedSizes


    def setFragmentSize(self,fragSize,fragSizeRMS):
        try:
            for application in self._config.applications['FEROL']:
                fedSizes = self.getFedSizes(fragSize,fragSizeRMS,application,self.sendsToEVM(application))
                messengers.setParam("Event_Length_bytes_FED0","unsignedInt",str(fedSizes[0]),**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED0","unsignedInt",str(fedSizes[1]),**application)
                messengers.setParam("Event_Length_bytes_FED1","unsignedInt",str(fedSizes[2]),**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED1","unsignedInt",str(fedSizes[3]),**application)
        except KeyError:
             for application in self._config.applications['EVM']:
                    fedSizes = self.getFedSizes(fragSize,fragSizeRMS,application,True)
                    messengers.setParam("dummyFedSize","unsignedInt",str(fedSizes[0]),**application)
                    messengers.setParam("dummyFedSizeStdDev","unsignedInt",str(fedSizes[1]),**application)
             for application in self._config.applications['RU']:
                    fedSizes = self.getFedSizes(fragSize,fragSizeRMS,application,False)
                    messengers.setParam("dummyFedSize","unsignedInt",str(fedSizes[0]),**application)
                    messengers.setParam("dummyFedSizeStdDev","unsignedInt",str(fedSizes[1]),**application)


    def start(self,dropAtRU):
        runNumber=time.strftime("%s",time.localtime())
        self.configureEvB(maxTries=30)
        self.enableEvB(maxTries=30,runNumber=runNumber)
        self.checkRate(dropAtRU)


    def doIt(self,fragSize,fragSizeRMS,args):
        dataPoints = []
        self.setFragmentSize(fragSize,fragSizeRMS)
        self.start(args['dropAtRU'])
        if args['nbMeasurements'] == 0:
            dataPoints.append( self.getDataPoint() )
            raw_input("Press Enter to stop...")
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


    def runScan(self,fragSize,fragSizeRMS,args):
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
                return dataPoints
            except (FailedState,StateException,ValueException) as e:
                print(" failed: "+str(e))
                while tries < 10:
                    tries += 1
                    try:
                        self.haltEvB()
                        break
                    except FailedState as e:
                        print(" failed: "+str(e))
        raise e
