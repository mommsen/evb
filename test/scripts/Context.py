import os
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Application
import SymbolMap
import XMLtools


class Context:
    ptInstance = 0

    def __init__(self):
        self.role = None
        self.hostinfo = None
        self.policy = None
        self.applications = [Application.Application('xmem::probe::Application',Context.ptInstance,[]),]


    def getContext(self,ns,fullConfig=True):
        context = ET.Element(QN(ns,'Context'))
        context.set('url',"http://%(soapHostname)s:%(soapPort)s" % self.hostinfo)
        if fullConfig and self.policy is not None:
            context.append(self.policy)
        for app in self.applications:
            if fullConfig:
                app.addInContext(context,ns)
            else:
                app.addI2OEndpointInContext(context,ns)
                if app.params['class'].startswith('evb::'):
                    context.append( app.getApplicationContext(ns) )
        return context


    def addPtUtcpApplication(self):
        properties = [
            ('protocol','string','atcp'),
            ('maxClients','unsignedInt','9'),
            ('autoConnect','boolean','false'),
            ('ioQueueSize','unsignedInt','65536'),
            ('eventQueueSize','unsignedInt','65536'),
            ('maxReceiveBuffers','unsignedInt','12'),
            ('maxBlockSize','unsignedInt','65537')
            ]
        app = Application.Application('pt::utcp::Application',Context.ptInstance,properties)
        Context.ptInstance += 1
        app.params['protocol'] = 'atcp'
        app.params['i2oHostname'] = self.hostinfo['i2oHostname']
        app.params['i2oPort'] =  self.hostinfo['i2oPort']
        app.params['network'] = 'tcp'
        self.applications.append(app)


class FEROL(Context):
    instance = 0

    def __init__(self,symbolMap,destination,fedId,properties=[]):
        Context.__init__(self)
        self.role = 'FEROL'
        self.hostinfo = symbolMap.getHostInfo('FEROL'+str(FEROL.instance))
        if FEROL.instance % 2:
            frlPort = 'frlPort'
        else:
            frlPort = 'frlPort2'
        prop = [
            ('fedId','unsignedInt',str(fedId)),
            ('sourceHost','string',self.hostinfo['frlHostname']),
            ('sourcePort','unsignedInt',str(self.hostinfo[frlPort])),
            ('destinationHost','string',destination.hostinfo['frlHostname']),
            ('destinationPort','unsignedInt',str(destination.hostinfo[frlPort]))
            ]
        prop.extend(properties)
        app = Application.Application('evb::test::DummyFEROL',FEROL.instance,prop)
        self.applications.append(app)
        FEROL.instance += 1

        ferolSource = {}
        ferolSource['frlHostname'] = self.hostinfo['frlHostname']
        ferolSource['frlPort'] = self.hostinfo[frlPort]
        ferolSource['fedId'] = str(fedId)
        for app in destination.applications:
            app.addFerolSource(ferolSource)


    def __del__(self):
        FEROL.instance = 0


class RU(Context):
    instance = 0

    def __init__(self,symbolMap,properties=[]):
        Context.__init__(self)
        self.hostinfo = symbolMap.getHostInfo('RU'+str(RU.instance))
        self.addPtUtcpApplication()
        self.addRuApplication(properties)
        RU.instance += 1


    def __del__(self):
        RU.instance = 0


    def addRuApplication(self,properties):
        if RU.instance == 0:
            self.role = 'EVM'
            app = Application.Application('evb::EVM',RU.instance,properties)
            app.params['tid'] = '1'
            app.params['id'] = '50'
        else:
            app = Application.Application('evb::RU',RU.instance,properties)
            app.params['tid'] = str(10+RU.instance)
            app.params['id'] = str(50+RU.instance)
            self.role = 'RU'
        app.params['network'] = 'tcp'
        inputSource = app.getInputSource()
        if inputSource == "Socket":
            self.addPtBlitApplication()
        self.applications.append(app)


    def addPtBlitApplication(self):
        maxBlockSize='0x10000'
        properties = [
            ('maxClients','unsignedInt','32'),
            ('maxReceiveBuffers','unsignedInt','128'),
            ('maxBlockSize','unsignedInt',maxBlockSize)
            ]
        app = Application.Application('pt::blit::Application',Context.ptInstance,properties)
        Context.ptInstance += 1
        app.params['frlHostname'] = self.hostinfo['frlHostname']
        app.params['frlPort'] = self.hostinfo['frlPort']
        app.params['frlPort2'] = self.hostinfo['frlPort2']
        app.params['maxbulksize'] = maxBlockSize
        self.applications.append(app)


class BU(Context):
    instance = 0

    def __init__(self,symbolMap,properties=[]):
        Context.__init__(self)
        self.role = 'BU'
        self.hostinfo = symbolMap.getHostInfo('BU'+str(BU.instance))
        self.addPtUtcpApplication()
        self.addBuApplication(properties)
        BU.instance += 1


    def __del__(self):
        BU.instance = 0


    def addBuApplication(self,properties):
        app = Application.Application('evb::BU',BU.instance,properties)
        app.params['tid'] = str(100+BU.instance)
        app.params['id'] = str(100+BU.instance)
        app.params['network'] = 'tcp'
        self.applications.append(app)


def resetInstanceNumbers():
    # resets all instance numbers of the above classes
    # (these are class wide / static variables)
    FEROL.instance = 0
    RU.instance = 0
    BU.instance = 0

if __name__ == "__main__":
    os.environ["EVB_SYMBOL_MAP"] = 'standaloneSymbolMap.txt'
    symbolMap = SymbolMap.SymbolMap(os.environ["EVB_TESTER_HOME"]+"/cases/")
    xcns = 'http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30'
    partition = ET.Element(QN(xcns,'Partition'))
    evm = RU(symbolMap,[
        ('inputSource','string','Socket')
        ])
    for fedId in range(3):
        ferol = FEROL(symbolMap,evm,fedId)
        partition.append( ferol.getContext(xcns) )
    partition.append( evm.getContext(xcns) )

    ru1 = RU(symbolMap,[
        ('inputSource','string','Local'),
        ('fedSourceIds','unsignedInt',range(6,10))
        ])
    partition.append( ru1.getContext(xcns) )

    ru2 = RU(symbolMap,[
        ('inputSource','string','FEROL')
        ])
    for fedId in range(10,12):
        ferol = FEROL(symbolMap,ru2,fedId)
        partition.append( ferol.getContext(xcns) )
    partition.append( ru2.getContext(xcns) )

    bu = BU(symbolMap,[
        ('dropEventData','boolean','true'),
        ('lumiSectionTimeout','unsignedInt','0')
        ])
    partition.append( bu.getContext(xcns) )

    XMLtools.indent(partition)
    print( ET.tostring(partition) )
