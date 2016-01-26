import sys
import time

import messengers
from TestCase import *


class ConfigCase(TestCase):

    def __init__(self,config,stdout):
        TestCase.__init__(self,config,stdout)


    def __del__(self):
        TestCase.__del__(self)


    def checkRate(self):
        print("Checking if rate is >10 Hz:")
        try:
            self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"EVM")
            self.checkAppParam("eventRate","unsignedInt",10,operator.ge,"RU")
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


    def getBandwidthMB(self,dataPoint):
        size = sum(list(x['sizes']['RU1'] for x in dataPoint))/float(len(dataPoint))
        rate = sum(list(x['rates']['RU1'] for x in dataPoint))/float(len(dataPoint))
        return int(size*rate/1000000)


    def sendsToEVM(self,application):
        for app in self._config.contexts[(application['soapHostname'],application['soapPort'])].applications:
            if app.params['class'] == 'ferol::FerolController':
                return app.params['sendsToEVM']
        return False


    def setFragmentSize(self,fragSize,fragSizeRMS):
        for application in self._config.applications['FEROL']:
            if self.sendsToEVM(application):
                messengers.setParam("Event_Length_bytes_FED0","unsignedInt","1024",**application)
                messengers.setParam("Event_Length_bytes_FED1","unsignedInt","1024",**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED0","unsignedInt","0",**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED1","unsignedInt","0",**application)
            else:
                messengers.setParam("Event_Length_bytes_FED0","unsignedInt",str(fragSize),**application)
                messengers.setParam("Event_Length_bytes_FED1","unsignedInt",str(fragSize),**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED0","unsignedInt",str(fragSizeRMS),**application)
                messengers.setParam("Event_Length_Stdev_bytes_FED1","unsignedInt",str(fragSizeRMS),**application)


    def start(self):
        self.configureEvB(maxTries=30)
        self.enableEvB(maxTries=30)
        self.checkRate()


    def doIt(self,fragSize,fragSizeRMS,nbMeasurements):
        dataPoints = []
        self.setFragmentSize(fragSize,fragSizeRMS)
        self.start()
        if nbMeasurements == 0:
            dataPoints.append( self.getDataPoint() )
            raw_input("Press Enter to stop...")
        else:
            for n in range(nbMeasurements):
                time.sleep(1)
                dataPoints.append( self.getDataPoint() )
        self.haltEvB()
        return dataPoints


    def runScan(self,fragSize,fragSizeRMS,nbMeasurements,verbose):
        if not verbose:
            self._origStdout.write(str(fragSize)+"B:")
            self._origStdout.flush()
        tries = 0
        while tries < 10:
            try:
                dataPoints = self.doIt(fragSize,fragSizeRMS,nbMeasurements)
                if not verbose:
                    self._origStdout.write(str(self.getBandwidthMB(dataPoints))+"MB/s ")
                    self._origStdout.flush()
                else:
                    print(str(fragSize)+"B:"+str(self.getBandwidthMB(dataPoints))+"MB/s")
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
