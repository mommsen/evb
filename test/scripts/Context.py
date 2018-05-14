import os
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Application
import SymbolMap
import XMLtools


class Context:
    ptInstance = 0

    def __init__(self,role,hostinfo):
        self.role = role
        self.hostinfo = hostinfo
        self.polns = 'http://xdaq.web.cern.ch/xdaq/xsd/2013/XDAQPolicy-10'
        self.policyElements = self.getPolicyElements()
        self.applications = [Application.Application('xmem::probe::Application',Context.ptInstance,[]),]


    def getPolicyElements(self):
        return []


    def getContext(self,ns,useNuma,fullConfig=True):
        context = ET.Element(QN(ns,'Context'))
        context.set('url',"http://%(soapHostname)s:%(soapPort)s" % self.hostinfo)
        if useNuma and fullConfig:
            self.addPolicy(context)
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


    def addPolicy(self,context):
        if self.policyElements:
            policy = ET.Element(QN(self.polns,'policy'))
            for element in self.policyElements:
                el = ET.Element(QN(self.polns,'element'),element)
                policy.append(el)
            context.append(policy)


    def extractPolicy(self,context):
        policy = context.find(QN(self.polns,'policy').text)
        if policy is not None:
            for element in policy:
                self.policyElements.append(element.attrib)


class FEROL(Context):
    instance = 0

    def __init__(self,symbolMap,destination,fedId,properties=[]):
        Context.__init__(self,'FEROL',symbolMap.getHostInfo('FEROL'+str(FEROL.instance)))
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
        Context.__init__(self,None,symbolMap.getHostInfo('RU'+str(RU.instance)))
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


    def getPolicyElements(self):
        # extracted from XML policy with
        # egrep -o '[[:alnum:]]+=".*"' scripts/tmp.txt |sed -re 's/([[:alnum:]]+)=/"\1":/g'|tr "\"" "'"|tr " " ","|sed -re 's/(.*)/{\1},/'
        policyElements = [
            {'cpunodes':'1','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::acceptor(.*)/waiting','type':'thread'},
            {'cpunodes':'1','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::eventworkloop/polling','type':'thread'},
            {'affinity':'3','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'affinity':'7','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-receiver(.+*)-mlx4_0:ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-sender(.*)-mlx4_0:ibvla','type':'alloc'},
            {'affinity':'16','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_2/waiting','type':'thread'},
            {'affinity':'24','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_3/waiting','type':'thread'},
            {'affinity':'26','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_4/waiting','type':'thread'},
            {'affinity':'20','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_5/waiting','type':'thread'},
            {'cpunodes':'0','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/monitoring/waiting','type':'thread'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:fragmentRequestFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:fragmentFIFO_FED(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:frameFIFO_BU(.+)','type':'alloc'},
            {'affinity':'9','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['frlPort'])+'/(.*)','type':'thread'},
            {'affinity':'13','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['frlPort2'])+'/(.*)','type':'thread'},
            {'affinity':'9','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort'])+'/waiting','type':'thread'},
            {'affinity':'13','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort2'])+'/waiting','type':'thread'},
            {'affinity':'15','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-ia(.+)/'+str(self.hostinfo['frlHostname'])+':btcp/(.*)','type':'thread'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:toolbox-mem-allocator-blit-socket(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:pt-blit-inputpipe-rlist(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:socketBufferFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:grantFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:tcpla-(.*)/'+str(self.hostinfo['frlHostname'])+'(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:pt::ibv::(.*)','type':'alloc'}
        ]
        if RU.instance == 0: #EVM
            policyElements.extend([
                {'affinity':'2,6,12,14,18,22,28,30','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/parseSocketBuffers_(.*)/waiting','type':'thread'},
                {'affinity':'2,6,12,14,18,22,28,30','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/generating_(.*)/waiting','type':'thread'},
                {'affinity':'4','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_0/waiting','type':'thread'},
                {'affinity':'8','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_1/waiting','type':'thread'},
                {'affinity':'10','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/processRequests/waiting','type':'thread'},
                {'affinity':'24','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/buPoster/waiting','type':'thread'},
                {'affinity':'24','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/dummySuperFragment/waiting','type':'thread'},
                {'mempolicy':'onnode','node':'0','package':'numa','pattern':'urn:readoutMsgFIFO(.+)','type':'alloc'}
                ])
        else: #RU
            policyElements.extend([
                {'affinity':'0,2,6,12,14,18,22,28,30','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/parseSocketBuffers_(.*)/waiting','type':'thread'},
                {'affinity':'0,2,6,12,14,18,22,28,30','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/generating_(.*)/waiting','type':'thread'},
                {'affinity':'8','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_0/waiting','type':'thread'},
                {'affinity':'10','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_1/waiting','type':'thread'},
                {'affinity':'4','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/buPoster/waiting','type':'thread'},
                {'affinity':'4','memnode':'0','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/dummySuperFragment/waiting','type':'thread'}
                ])
        return policyElements


class BU(Context):
    instance = 0

    def __init__(self,symbolMap,properties=[]):
        Context.__init__(self,'BU',symbolMap.getHostInfo('BU'+str(BU.instance)))
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


    def getPolicyElements(self):
        policyElements = [
            {'affinity':'3','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_0/waiting','type':'thread'},
            {'affinity':'5','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_1/waiting','type':'thread'},
            {'affinity':'7','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_2/waiting','type':'thread'},
            {'affinity':'9','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_3/waiting','type':'thread'},
            {'affinity':'11','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_4/waiting','type':'thread'},
            {'affinity':'13','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_5/waiting','type':'thread'},
            {'affinity':'31','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/requestFragments/waiting','type':'thread'},
            {'affinity':'29','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/fileMover/waiting','type':'thread'},
            {'affinity':'27','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/lumiAccounting/waiting','type':'thread'},
            {'affinity':'23','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/resourceMonitor/waiting','type':'thread'},
            {'affinity':'25','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/monitoring/waiting','type':'thread'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:superFragmentFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:fileStatisticsFIFO_stream(.+):alloc','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:resourceFIFO:alloc','type':'alloc'},
            {'cpunodes':'1','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::acceptor-(.+)/waiting','type':'thread'},
            {'cpunodes':'1','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::eventworkloop/polling','type':'thread'},
            {'affinity':'10','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'affinity':'12','memnode':'1','mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:toolbox-mem-allocator-ibv(.*):ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:pt::ibv::(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':'1','package':'numa','pattern':'urn:undefined','type':'alloc'}
            ]
        return policyElements


def resetInstanceNumbers():
    # resets all instance numbers of the above classes
    # (these are class wide / static variables)
    FEROL.instance = 0
    RU.instance = 0
    BU.instance = 0


if __name__ == "__main__":
    useNuma=True
    symbolMap = SymbolMap.SymbolMap(os.environ["EVB_TESTER_HOME"]+"/cases/standaloneSymbolMap.txt")
    xcns = 'http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30'
    partition = ET.Element(QN(xcns,'Partition'))
    evm = RU(symbolMap,[
        ('inputSource','string','Socket')
        ])
    for fedId in range(3):
        ferol = FEROL(symbolMap,evm,fedId)
        partition.append( ferol.getContext(xcns,useNuma) )
    partition.append( evm.getContext(xcns,useNuma) )

    ru1 = RU(symbolMap,[
        ('inputSource','string','Local'),
        ('fedSourceIds','unsignedInt',range(6,10))
        ])
    partition.append( ru1.getContext(xcns,useNuma) )

    ru2 = RU(symbolMap,[
        ('inputSource','string','FEROL')
        ])
    for fedId in range(10,12):
        ferol = FEROL(symbolMap,ru2,fedId)
        partition.append( ferol.getContext(xcns,useNuma) )
    partition.append( ru2.getContext(xcns,useNuma) )

    bu = BU(symbolMap,[
        ('dropEventData','boolean','true'),
        ('lumiSectionTimeout','unsignedInt','0')
        ])
    partition.append( bu.getContext(xcns,useNuma) )

    XMLtools.indent(partition)
    print( ET.tostring(partition) )
