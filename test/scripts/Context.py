import os
import xmlrpclib
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
        try:
            self.policyElements = self.getPolicyElements()
        except KeyError:
            self.policyElements = []
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


    def addPeerTransport(self):
        try:
            if 'rbs1v0' in self.hostinfo['i2oHostname'] or 'ebs0v0' in self.hostinfo['i2oHostname'] or 'ebs1v0' in self.hostinfo['i2oHostname']:
                app = self.getPtIbvApplication()
            else:
                app = self.getPtUtcpApplication()
            app.params['i2oHostname'] = self.hostinfo['i2oHostname']
            app.params['i2oPort'] =  self.hostinfo['i2oPort']
            app.params['network'] = 'evb'
            self.applications.append(app)
            Context.ptInstance += 1
        except KeyError as e:
            raise KeyError("Cannot find key "+str(e)+" for "+self.hostinfo['soapHostname'])


    def getPtUtcpApplication(self):
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
        app.params['protocol'] = 'atcp'
        return app


    def getPtIbvApplication(self):
        if 'dvrubu-c2e3' in self.hostinfo['i2oHostname']:
            interface='mlx5_0'
        elif 'dvrubu-c2f33-1' in self.hostinfo['i2oHostname']:
            interface='mlx4_1'
        else:
            interface='mlx4_0'
        properties = [
            ('iaName','string',interface),
            ('sendPoolName','string','sudapl'),
            ('recvPoolName','string','rudapl'),
            ('deviceMTU','unsignedInt','4096'),
            ('memAllocTimeout','string','PT1S'),
            ('sendWithTimeout','boolean','true'),
            ('useRelay','boolean','false'),
            ('maxMessageSize','unsignedInt','0x20000')
            ]
        if self.role is 'EVM':
            properties.extend([
                ('senderPoolSize','unsignedLong','0x2A570C00'),
                ('receiverPoolSize','unsignedLong','0x5D70A400'),
                ('completionQueueSize','unsignedInt','482160'),
                ('sendQueuePairSize','unsignedInt','5840'),
                ('recvQueuePairSize','unsignedInt','80')
            ])
        elif self.role is 'RU':
            properties.extend([
                ('senderPoolSize','unsignedLong','0xBAE14800'),
                ('receiverPoolSize','unsignedLong','0x5D70A400'),
                ('completionQueueSize','unsignedInt','23860'),
                ('sendQueuePairSize','unsignedInt','320'),
                ('recvQueuePairSize','unsignedInt','500')
            ])
        elif self.role is 'BU':
            properties.extend([
                ('senderPoolSize','unsignedLong','0xA3D800'),
                ('receiverPoolSize','unsignedLong','0x1788E3800'),
                ('completionQueueSize','unsignedInt','32400'),
                ('sendQueuePairSize','unsignedInt','80'),
                ('recvQueuePairSize','unsignedInt','320')
            ])
        elif self.role is 'RUBU':
            properties.extend([
                ('senderPoolSize','unsignedLong','0xBB852000'),
                ('receiverPoolSize','unsignedLong','0x200000000'), #maximum pool size with 65536 buffers of 131072 bytes
                ('completionQueueSize','unsignedInt','32400'),
                ('sendQueuePairSize','unsignedInt','320'),
                ('recvQueuePairSize','unsignedInt','320')
            ])

        app = Application.Application('pt::ibv::Application',Context.ptInstance,properties)
        app.params['protocol'] = 'ibv'
        return app


    def getNumaInfo(self):
        try:
            server = xmlrpclib.ServerProxy("http://%s:%s" % (self.hostinfo['soapHostname'],self.hostinfo['launcherPort']))
            return server.getNumaInfo()
        except xmlrpclib.Fault:
            return {
                '1': ['1', '3', '5', '7', '9', '11', '13', '15', '17', '19', '21', '23', '25', '27', '29', '31'],
                '0': ['0', '2', '4', '6', '8', '10', '12', '14', '16', '18', '20', '22', '24', '26', '28', '30'],
                'ethCPU': '1', 'ibvCPU': '0'
                }
        except Exception as e:
            raise Exception("Failed to get NUMA information from "+self.hostinfo['soapHostname']+": "+str(e))


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
        if RU.instance == 0:
            role = 'EVM'
        else:
            role = 'RU'
        Context.__init__(self,role,symbolMap.getHostInfo('RU'+str(RU.instance)))
        self.addPeerTransport()
        self.addRuApplication(properties)
        RU.instance += 1


    def addRuApplication(self,properties):
        if self.role is 'EVM':
            app = Application.Application('evb::EVM',RU.instance,properties)
            app.params['tid'] = '1'
            app.params['id'] = '50'
        else:
            app = Application.Application('evb::RU',RU.instance,properties)
            app.params['tid'] = str(10+RU.instance)
            app.params['id'] = str(50+RU.instance)
        app.params['network'] = 'evb'
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
        numaInfo = self.getNumaInfo()
        #print self.hostinfo['soapHostname'],numaInfo

        # extracted from XML policy with
        # egrep -o '[[:alnum:]]+=".*"' scripts/tmp.txt |sed -re 's/([[:alnum:]]+)=/"\1":/g'|tr "\"" "'"|tr " " ","|sed -re 's/(.*)/{\1},/'
        policyElements = [
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::acceptor(.*)/waiting','type':'thread'},
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::eventworkloop/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][1],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][3],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][8],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_2/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][12],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_3/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][13],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_4/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][10],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_5/waiting','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-receiver(.+*):ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-sender(.*):ibvla','type':'alloc'},
            {'cpunodes':numaInfo['ibvCPU'],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/monitoring/waiting','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:fragmentRequestFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:fragmentFIFO_FED(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:frameFIFO_BU(.+)','type':'alloc'},
            {'affinity':numaInfo[numaInfo['ethCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-ia(.+)/'+str(self.hostinfo['i2oHostname'])+':btcp/(.*)','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-blit-socket(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:pt-blit-inputpipe-rlist(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:socketBufferFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:grantFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:tcpla-(.*)/'+str(self.hostinfo['i2oHostname'])+'(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:pt::ibv::(.*)','type':'alloc'}
        ]
        try:
            policyElements.extend([
                {'affinity':numaInfo[numaInfo['ethCPU']][4],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['i2oPort'])+'/(.*)','type':'thread'},
                {'affinity':numaInfo[numaInfo['ethCPU']][6],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['frlPort2'])+'/(.*)','type':'thread'},
                {'affinity':numaInfo[numaInfo['ethCPU']][4],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort'])+'/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ethCPU']][6],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort2'])+'/waiting','type':'thread'}
                ])
        except KeyError:
            pass

        if RU.instance == 0: #EVM
            workerCores = (1,3,6,9,11,12,14,15)
            policyElements.extend([
                {'affinity':numaInfo[numaInfo['ibvCPU']][2],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_0/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][4],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_1/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][5],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/processRequests/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/buPoster/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/dummySuperFragment/waiting','type':'thread'},
                {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:readoutMsgFIFO(.+)','type':'alloc'}
                ])
        else: #RU
            workerCores = (0,1,3,6,7,9,11,14,15)
            policyElements.extend([
                {'affinity':numaInfo[numaInfo['ibvCPU']][4],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_0/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][5],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_1/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][2],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/buPoster/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][2],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/dummySuperFragment/waiting','type':'thread'}
                ])
        policyElements.extend([
                {'affinity':','.join([numaInfo[numaInfo['ibvCPU']][i] for i in workerCores]),'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/parseSocketBuffers_(.*)/waiting','type':'thread'},
                {'affinity':','.join([numaInfo[numaInfo['ibvCPU']][i] for i in workerCores]),'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/generating_(.*)/waiting','type':'thread'}
                ])
        return policyElements


class BU(Context):
    instance = 0

    def __init__(self,symbolMap,properties=[]):
        Context.__init__(self,'BU',symbolMap.getHostInfo('BU'+str(BU.instance)))
        self.addPeerTransport()
        self.addBuApplication(properties)
        BU.instance += 1


    def addBuApplication(self,properties):
        app = Application.Application('evb::BU',BU.instance,properties)
        app.params['tid'] = str(100+BU.instance)
        app.params['id'] = str(100+BU.instance)
        app.params['network'] = 'evb'
        self.applications.append(app)


    def getPolicyElements(self):
        numaInfo = self.getNumaInfo()
        #print self.hostinfo['soapHostname'],numaInfo

        policyElements = [
            {'affinity':numaInfo[numaInfo['ethCPU']][1],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_0/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][2],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_1/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][3],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_2/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][4],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_3/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][5],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_4/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][6],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_5/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][15],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/requestFragments/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][14],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/fileMover/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][13],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/lumiAccounting/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][11],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/resourceMonitor/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][12],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/monitoring/waiting','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:superFragmentFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:fileStatisticsFIFO_stream(.+):alloc','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:resourceFIFO:alloc','type':'alloc'},
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::acceptor-(.+)/waiting','type':'thread'},
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::eventworkloop/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][5],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][6],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-ibv(.*):ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:pt::ibv::(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:undefined','type':'alloc'}
            ]
        return policyElements


class RUBU(RU):

    def __init__(self,symbolMap,ruProperties=[],buProperties=[]):
        Context.__init__(self,'RUBU',symbolMap.getHostInfo('RU'+str(RU.instance)))
        self.addPeerTransport()
        self.addRuApplication(ruProperties)
        RU.instance += 1
        self.addBuApplication(buProperties)


    def addBuApplication(self,properties):
        app = Application.Application('evb::BU',RU.instance-1,properties)
        app.params['tid'] = str(199+RU.instance)
        app.params['id'] = str(199+RU.instance)
        app.params['network'] = 'evb'
        self.applications.append(app)


    def getPolicyElements(self):
        numaInfo = self.getNumaInfo()
        socketThreads = (8,9,15)
        policyElements = [
            {'affinity':','.join([numaInfo[numaInfo['ibvCPU']][i] for i in socketThreads]),'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/parseSocketBuffers_(.*)/waiting','type':'thread'},
            {'affinity':','.join([numaInfo[numaInfo['ibvCPU']][i] for i in socketThreads]),'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/generating_(.*)/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][2],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_0/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][3],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_1/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][5],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][6],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_2/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][10],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_3/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][11],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_4/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][14],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Responder_5/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ibvCPU']][14],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:fifo/PeerTransport/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][0],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloops(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][0],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/buPoster/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][1],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_5/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][2],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_0/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][3],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_1/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][5],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_2/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][9],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/requestFragments/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][10],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_3/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][11],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/resourceMonitor/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][12],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::completionworkloopr(.*)/polling','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][13],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Builder_4/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][14],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/fileMover/waiting','type':'thread'},
            {'affinity':numaInfo[numaInfo['ethCPU']][15],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/lumiAccounting/waiting','type':'thread'},
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::acceptor(.*)/waiting','type':'thread'},
            {'cpunodes':numaInfo['ethCPU'],'memnode':numaInfo['ethCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:pt::ibv::eventworkloop/polling','type':'thread'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:superFragmentFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:fileStatisticsFIFO_stream(.+):alloc','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:resourceFIFO:alloc','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-receiver(.+*):ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-ibv-sender(.*):ibvla','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:pt::ibv::(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:fragmentRequestFIFO(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:fragmentFIFO_FED(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:frameFIFO_BU(.+)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:toolbox-mem-allocator-blit-socket(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:pt-blit-inputpipe-rlist(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:socketBufferFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ibvCPU'],'package':'numa','pattern':'urn:grantFIFO(.*)','type':'alloc'},
            {'mempolicy':'onnode','node':numaInfo['ethCPU'],'package':'numa','pattern':'urn:fifo-PeerTransport:alloc','type':'alloc'}
            ]

        try:
            policyElements.extend([
                {'affinity':numaInfo[numaInfo['ethCPU']][0],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['i2oPort'])+'/(.*)','type':'thread'},
                {'affinity':numaInfo[numaInfo['ethCPU']][0],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:tcpla-psp(.*)/'+str(self.hostinfo['frlHostname'])+'([:/])'+str(self.hostinfo['frlPort2'])+'/(.*)','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort'])+'/waiting','type':'thread'},
                {'affinity':numaInfo[numaInfo['ibvCPU']][7],'memnode':numaInfo['ibvCPU'],'mempolicy':'onnode','package':'numa','pattern':'urn:toolbox-task-workloop:evb::(.+)/Pipe_'+str(self.hostinfo['frlHostname'])+':'+str(self.hostinfo['frlPort2'])+'/waiting','type':'thread'}
                ])
        except KeyError:
            pass

        return policyElements


def resetInstanceNumbers():
    # resets all instance numbers of the above classes
    # (these are class wide / static variables)
    Context.ptInstance = 0
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

    rubu = RUBU(symbolMap,[
        ('inputSource','string','Local'),
        ('fedSourceIds','unsignedInt',range(6,10))],
        [('dropEventData','boolean','true'),
        ('lumiSectionTimeout','unsignedInt','0')
        ])
    partition.append( rubu.getContext(xcns,useNuma) )

    XMLtools.indent(partition)
    print( ET.tostring(partition) )
