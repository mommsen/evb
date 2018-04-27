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


    def checkRate(self,dropAtRU,dropAtSocket):
        print("Checking if rate is >10 Hz:")
        try:
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
        if len(self.fedSizeScaleFactors):
            try:
                (a,b,c,rms) = self.fedSizeScaleFactors[fedId]
                relSize = fragSize / ConfigCase.defaultFedSize
                fedSize = a + b*relSize + c*relSize*relSize
                fedSizeRMS = fedSize * rms
                return (int(fedSize+4)&~0x7,int(fedSizeRMS+4)&~0x7)
            except KeyError:
                if fedId != '0xffffffff':
                    print("Missing scale factor for FED id "+fedId)
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


    def start(self,dropAtRU,dropAtSocket):
        """
        configures and enables (starts) the event builder 
        """

        runNumber=time.strftime("%s",time.localtime())
        self.configureEvB(maxTries=30)
        self.enableEvB(maxTries=30,runNumber=runNumber)
        self.checkRate(dropAtRU,dropAtSocket)


    def doIt(self,fragSize,fragSizeRMS,args):
        """
        runs a test for a single fragment size 
        and returns the obtained performance numbers
        """

        dataPoints = []
        self.setFragmentSizes(fragSize,fragSizeRMS)

        # start running
        self.start(args['dropAtRU'],args['dropAtSocket'])

        if args['nbMeasurements'] == 0:
            dataPoints.append( self.getDataPoint() )

            # run until the user presses enter
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
