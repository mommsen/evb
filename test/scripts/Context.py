import copy
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import SymbolMap
import XMLtools

id = 11

class Context:
    ptInstance = 0

    def __init__(self,params=[],nestedContexts=()):
        global id
        self._params = copy.deepcopy(params)
        self.nestedContexts = nestedContexts
        self.role = None
        self.apps = {
            'app':None,
            'instance':None,
            'appId':id,
            'ptInstance':Context.ptInstance
            }
        Context.ptInstance += 1
        id += 1


    def __del__(self):
        Context.ptInstance = 0


    def addInContext(self,context,ns):

        self.addConfigForPeerTransport(context,ns)
        self.addConfigForPtUtcp(context,ns)
        self.addConfigForApplication(context,ns)


    def addConfigForApplication(self,context,ns):
        application = self.getApplicationContext(ns)
        application.append( self.getProperties(self.apps['app'],self._params) )
        context.append(application)
        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libtcpla.so"
        context.append(module)
        if self.apps['app'].startswith('evb::'):
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_LOCAL/lib/libevb.so"
            context.append(module)


    def addConfigForPeerTransport(self,context,ns):
        pass


    def addConfigForPtUtcp(self,context,ns):
        global id
        try:
            context.append( self.getEndpointContext(ns) )

            utcp = ET.Element(QN(ns,'Application'))
            utcp.set('class','pt::utcp::Application')
            utcp.set('id',str(id))
            utcp.set('instance',str(self.apps['ptInstance']))
            utcp.set('network',self.apps['network'])
            utcp.append( self.getProperties('pt::utcp::Application',[
                ('protocol','string','atcp'),
                ('maxClients','unsignedInt','9'),
                ('autoConnect','boolean','false'),
                ('ioQueueSize','unsignedInt','65536'),
                ('eventQueueSize','unsignedInt','65536'),
                ('maxReceiveBuffers','unsignedInt','12'),
                ('maxBlockSize','unsignedInt','65537')
                ]) )
            context.append(utcp)
            id += 1

            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_ROOT/lib/libtcpla.so"
            context.append(module)
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_ROOT/lib/libptutcp.so"
            context.append(module)

        except KeyError:
            pass


    def getApplicationContext(self,ns):
        application = ET.Element(QN(ns,'Application'))
        application.set('class',self.apps['app'])
        application.set('id',str(self.apps['appId']))
        application.set('instance',str(self.apps['instance']))
        try:
            application.set('network',self.apps['network'])
        except KeyError:
            application.set('network','local')
        return application


    def getEndpointContext(self,ns):
        endpoint = ET.Element(QN(ns,'Endpoint'))
        endpoint.set('protocol',self.apps['ptProtocol'])
        endpoint.set('service','i2o')
        endpoint.set('hostname',self.apps['i2oHostname'])
        endpoint.set('port',self.apps['i2oPort'])
        endpoint.set('network',self.apps['network'])
        return endpoint


    def getProperties(self,app,params):
        appns = "urn:xdaq-application:"+app
        soapencns = "http://schemas.xmlsoap.org/soap/encoding/"
        xsins = "http://www.w3.org/2001/XMLSchema-instance"
        properties = ET.Element(QN(appns,'properties'))
        properties.set(QN(xsins,'type'),'soapenc:Struct')
        for param in params:
            p = ET.Element(QN(appns,param[0]))
            p.set(QN(xsins,'type'),'soapenc:Array')
            if isinstance(param[2],tuple) or isinstance(param[2],list):
                p.set(QN(soapencns,'arrayType'),'xsd:ur-type['+str(len(param[2]))+']')
                for index,value in enumerate(param[2]):
                    item = ET.Element(QN(appns,'item'))
                    item.set(QN(soapencns,'position'),'['+str(index)+']')
                    if param[1] == 'Struct':
                        item.set(QN(xsins,'type'),'soapenc:Struct')
                        for val in value:
                            e = ET.Element(QN(appns,val[0]))
                            e.set(QN(xsins,'type'),'xsd:'+val[1])
                            e.text = str(val[2])
                            item.append(e)
                    else:
                        item.set(QN(xsins,'type'),'xsd:'+param[1])
                        item.text = str(value)
                    p.append(item)
            else:
                p.set(QN(xsins,'type'),'xsd:'+param[1])
                p.text = str(param[2])
            properties.append(p)
        return properties


class FEROL(Context):
    instance = 0

    def __init__(self,symbolMap,destination,fedId,params=[],nestedContexts=()):
        Context.__init__(self,params,nestedContexts)
        self.role = "FEROL"
        self.apps['app'] = "evb::test::DummyFEROL"
        self.apps['instance'] = FEROL.instance
        self.apps.update( symbolMap.getHostInfo('FEROL'+str(FEROL.instance)) )
        if FEROL.instance % 2:
            frlPort = 'frlPort'
        else:
            frlPort = 'frlPort2'
        FEROL.instance += 1

        self._params.append(('fedId','unsignedInt',str(fedId)))
        self._params.append(('sourceHost','string',self.apps['frlHostname']))
        self._params.append(('sourcePort','unsignedInt',str(self.apps['frlPort'])))
        self._params.append(('destinationHost','string',destination.apps['frlHostname']))
        self._params.append(('destinationPort','unsignedInt',str(destination.apps[frlPort])))

        ferolSource = dict((key, self.apps[key]) for key in ('frlHostname','frlPort'))
        ferolSource['fedId'] = str(fedId)
        destination.apps['ferolSources'].append(ferolSource)


    def __del__(self):
        FEROL.instance = 0



class RU(Context):
    instance = 0

    def __init__(self,symbolMap,params,nestedContexts=()):
        Context.__init__(self,params,nestedContexts)
        if RU.instance == 0:
            self.apps['app'] = "evb::EVM"
            self.role = "EVM"
        else:
            self.apps['app'] = "evb::RU"
            self.role = "RU"
        self.apps['instance'] = RU.instance
        self.apps.update( symbolMap.getHostInfo('RU'+str(RU.instance)) )
        self.apps['tid'] = str(RU.instance+1)
        self.apps['ptProtocol'] = "atcp"
        self.apps['network'] = "tcp"
        self.apps['ferolSources'] = []
        self.apps['inputSource'] = self.getInputSource()
        if self.apps['inputSource'] == 'FEROL':
            self.apps['ptFrl'] = "pt::frl::Application"
        RU.instance += 1
        self.routing = []


    def __del__(self):
        RU.instance = 0


    def fillFerolSources(self):
        if len(self.routing) == 0:
            ferolSources = []
            if len(self.apps['ferolSources']) == 0:
                for param in self._params:
                    try:
                        if param[0] == 'fedSourceIds':
                            for fedId in param[2]:
                               ferolSources.append( (
                                    ('fedId','unsignedInt',fedId),
                                    ('hostname','string','localhost'),
                                    ('port','unsignedInt','9999')
                                    ) )
                    except KeyError:
                        pass
            else:
                fedSourceIds = []
                for source in self.apps['ferolSources']:
                    fedSourceIds.append(source['fedId'])
                    ferolSources.append( (
                        ('fedId','unsignedInt',source['fedId']),
                        ('hostname','string',source['frlHostname']),
                        ('port','unsignedInt',source['frlPort'])
                        ) )
                    self.routing.append( (
                        ('fedid','string',source['fedId']),
                        ('className','string',self.apps['app']),
                        ('instance','string',str(self.apps['instance']))
                        ) )
                self._params.append( ('fedSourceIds','unsignedInt',fedSourceIds) );
            self._params.append( ('ferolSources','Struct',ferolSources) )


    def getInputSource(self):
        for param in self._params:
            try:
                if param[0] == 'inputSource':
                    return param[2]
            except KeyError:
                pass


    def addConfigForPeerTransport(self,context,ns):
        self.fillFerolSources()
        if self.apps['inputSource'] == 'FEROL':
            self.addConfigForPtFrl(context,ns)
        elif self.apps['inputSource'] == 'Socket':
            self.addConfigForPtBlit(context,ns)


    def addConfigForPtBlit(self,context,ns):
        global id
        endpoint = ET.Element(QN(ns,'Endpoint'))
        endpoint.set('protocol','btcp')
        endpoint.set('service','blit')
        endpoint.set('sndTimeout','2000')
        endpoint.set('rcvTimeout','0')
        endpoint.set('targetId','11')
        endpoint.set('singleThread','true')
        endpoint.set('pollingCycle','1')
        endpoint.set('rmode','select')
        endpoint.set('nonblock','true')
        endpoint.set('maxbulksize','131072')
        endpoint.set('hostname',self.apps['frlHostname'])
        endpoint.set('port',str(self.apps['frlPort']))
        endpoint.set('network','ferola')
        context.append(copy.deepcopy(endpoint))
        endpoint.set('port',self.apps['frlPort2'])
        endpoint.set('network','ferolb')
        context.append(endpoint)

        application = ET.Element(QN(ns,'Application'))
        application.set('class','pt::blit::Application')
        application.set('id',str(id))
        application.set('instance',str(self.apps['ptInstance']))
        application.set('network','local')
        id += 1

        application.append(self.getProperties('pt::blit::Application',[
            ('maxClients','unsignedInt','32'),
            ('maxReceiveBuffers','unsignedInt','128'),
            ('maxBlockSize','unsignedInt','131072')
            ]))
        context.append(application)

        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libtcpla.so"
        context.append(module)
        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libptblit.so"
        context.append(module)


    def addConfigForPtFrl(self,context,ns):
        global id
        endpoint = ET.Element(QN(ns,'Endpoint'))
        endpoint.set('protocol','ftcp')
        endpoint.set('service','frl')
        endpoint.set('sndTimeout','2000')
        endpoint.set('rcvTimeout','0')
        endpoint.set('singleThread','true')
        endpoint.set('pollingCycle','4')
        endpoint.set('rmode','select')
        endpoint.set('nonblock','true')
        endpoint.set('datagramSize','131072')
        endpoint.set('hostname',self.apps['frlHostname'])
        endpoint.set('port',str(self.apps['frlPort']))
        endpoint.set('network','ferola')
        context.append(copy.deepcopy(endpoint))
        endpoint.set('port',self.apps['frlPort2'])
        endpoint.set('network','ferolb')
        context.append(endpoint)

        application = ET.Element(QN(ns,'Application'))
        application.set('class','pt::frl::Application')
        application.set('id',str(id))
        application.set('instance',str(self.apps['ptInstance']))
        application.set('network','local')
        id += 1

        application.append(self.getProperties('pt::frl::Application',[
            ('frlRouting','Struct',self.routing),
            ('frlDispatcher','string','copy'),
            ('useUdaplPool','boolean','true'),
            ('autoConnect','boolean','false'),
            ('i2oFragmentBlockSize','unsignedInt','32768'),
            ('i2oFragmentsNo','unsignedInt','128'),
            ('i2oFragmentPoolSize','unsignedInt','10000000'),
            ('copyWorkerQueueSize','unsignedInt','16'),
            ('copyWorkersNo','unsignedInt','1'),
            ('inputStreamPoolSize','double','1400000'),
            ('maxClients','unsignedInt','10'),
            ('ioQueueSize','unsignedInt','64'),
            ('eventQueueSize','unsignedInt','64'),
            ('maxInputReceiveBuffers','unsignedInt','8'),
            ('maxInputBlockSize','unsignedInt','131072'),
            ('doSuperFragment','boolean','false')
            ]))
        context.append(application)

        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libtcpla.so"
        context.append(module)
        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libptfrl.so"
        context.append(module)


class BU(Context):
    instance = 0

    def __init__(self,symbolMap,params,nestedContexts=()):
        Context.__init__(self,params,nestedContexts)
        self.role = "BU"
        self.apps['app'] = "evb::BU"
        self.apps['instance'] = BU.instance
        self.apps.update( symbolMap.getHostInfo('BU'+str(BU.instance)) )
        self.apps['tid'] = str(BU.instance+30)
        self.apps['ptProtocol'] = "atcp"
        self.apps['network'] = "tcp"
        BU.instance += 1


    def __del__(self):
        BU.instance = 0



if __name__ == "__main__":
    context = Context()
    test1 = context.getProperties("foo",[
            ('maxClients','unsignedInt','32'),
            ('maxReceiveBuffers','unsignedInt','128'),
            ('maxBlockSize','unsignedInt','131072')
            ])
    XMLtools.indent(test1)
    print( ET.tostring(test1) )

    routing = []
    for id in 1,2,3,4,5:
        routing.append( (
            ('fedid','string',str(id)),
            ('className','string','foobar'),
            ('instance','string',str(id))
            ) )
    test2 = context.getProperties("bar",[
            ('frlRouting','Struct',routing),
            ('frlDispatcher','string','copy'),
            ('useUdaplPool','boolean','true'),
            ('autoConnect','boolean','false'),
            ('i2oFragmentBlockSize','unsignedInt','32768'),
            ('i2oFragmentsNo','unsignedInt','128'),
            ('i2oFragmentPoolSize','unsignedInt','10000000'),
            ('copyWorkerQueueSize','unsignedInt','16'),
            ('copyWorkersNo','unsignedInt','1'),
            ('inputStreamPoolSize','double','1400000'),
            ('maxClients','unsignedInt','10'),
            ('ioQueueSize','unsignedInt','64'),
            ('eventQueueSize','unsignedInt','64'),
            ('maxInputReceiveBuffers','unsignedInt','8'),
            ('maxInputBlockSize','unsignedInt','131072'),
            ('doSuperFragment','boolean','false')
            ])
    XMLtools.indent(test2)
    print( ET.tostring(test2) )
