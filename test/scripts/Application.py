import copy
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import XMLtools

id = 11

class Application:

    def __init__(self,className,instance,properties=[]):
        global id
        self.params= {}
        self.params['class'] = className
        self.params['instance'] = str(instance)
        self.params['network'] = "local"
        self.params['id'] = str(id)
        self.properties = properties
        id += 1


    def __del__(self):
        global id
        id = 11


    def addInContext(self,context,ns):
        self.addEndpointsInContext(context,ns)
        self.fillFerolSources()
        application = self.getApplicationContext(ns)
        application.append( self.getProperties() )
        context.append(application)
        self.addModulesInContext(context,ns)


    def getApplicationContext(self,ns):
        application = ET.Element(QN(ns,'Application'))
        application.set('class',self.params['class'])
        application.set('id',self.params['id'])
        application.set('instance',self.params['instance'])
        application.set('network',self.params['network'])
        return application


    def getProperties(self):
        appns = "urn:xdaq-application:"+self.params['class']
        soapencns = "http://schemas.xmlsoap.org/soap/encoding/"
        xsins = "http://www.w3.org/2001/XMLSchema-instance"
        properties = ET.Element(QN(appns,'properties'))
        properties.set(QN(xsins,'type'),'soapenc:Struct')
        for param in self.properties:
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


    def addModulesInContext(self,context,ns):
        module = ET.Element(QN(ns,'Module'))
        module.text = "$XDAQ_ROOT/lib/libtcpla.so"
        context.append(module)
        if self.params['class'].startswith('pt::utcp'):
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_ROOT/lib/libptutcp.so"
            context.append(module)
        elif self.params['class'].startswith('pt::blit'):
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_ROOT/lib/libptblit.so"
            context.append(module)
        elif self.params['class'].startswith('ferol::'):
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_ROOT/lib/libFerolController.so"
            context.append(module)
        elif self.params['class'].startswith('evb::'):
            module = ET.Element(QN(ns,'Module'))
            module.text = "$XDAQ_LOCAL/lib/libevb.so"
            context.append(module)


    def addTargetElement(self,protocol,ns):
        try:
            target = ET.Element(QN(ns,'target'))
            target.set('class',self.params['class'])
            target.set('instance',self.params['instance'])
            target.set('tid',self.params['tid'])
            protocol.append(target)
        except KeyError: #no TID
            pass


    def addEndpointsInContext(self,context,ns):
        self.addI2OEndpointInContext(context,ns)
        self.addPtBlitEndpointInContext(context,ns)


    def addI2OEndpointInContext(self,context,ns):
        try:
            endpoint = ET.Element(QN(ns,'Endpoint'))
            endpoint.set('protocol',self.params['protocol'])
            endpoint.set('service','i2o')
            endpoint.set('hostname',self.params['i2oHostname'])
            endpoint.set('port',self.params['i2oPort'])
            endpoint.set('network',self.params['network'])
            context.append(endpoint)
        except KeyError:
            pass


    def addPtBlitEndpointInContext(self,context,ns):
        try:
            endpoint = ET.Element(QN(ns,'Endpoint'))
            endpoint.set('protocol','btcp')
            endpoint.set('service','blit')
            endpoint.set('sndTimeout','2000')
            endpoint.set('rcvTimeout','0')
            endpoint.set('singleThread','true')
            endpoint.set('pollingCycle','1')
            endpoint.set('rmode','select')
            endpoint.set('nonblock','true')
            endpoint.set('maxbulksize','131072')
            endpoint.set('hostname',self.params['frlHostname'])
            endpoint.set('port',str(self.params['frlPort']))
            endpoint.set('network','ferola')
            context.append(copy.deepcopy(endpoint))
            endpoint.set('port',self.params['frlPort2'])
            endpoint.set('network','ferolb')
            context.append(endpoint)
        except KeyError:
            pass


    def addFerolSource(self,source):
        if self.params['class'] in ('evb::EVM','evb::RU'):
            if 'ferolSources' not in self.params:
                self.params['ferolSources'] = []
            self.params['ferolSources'].append(source)


    def fillFerolSources(self):
        for param in self.properties:
            if param[0] == 'ferolSources':
                return
        ferolSources = []
        try:
            fedSourceIds = []
            for source in self.params['ferolSources']:
                fedSourceIds.append(source['fedId'])
                ferolSources.append( (
                    ('fedId','unsignedInt',source['fedId']),
                    ('hostname','string',source['frlHostname']),
                    ('port','unsignedInt',source['frlPort'])
                    ) )
            self.properties.append( ('fedSourceIds','unsignedInt',fedSourceIds) );
        except KeyError:
            for param in self.properties:
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
        if len(ferolSources) > 0:
            self.properties.append( ('ferolSources','Struct',ferolSources) )


    def getInputSource(self):
        for param in self.properties:
            try:
                if param[0] == 'inputSource':
                    return param[2]
            except KeyError:
                pass


if __name__ == "__main__":
    properties = [
        ('maxClients','unsignedInt','32'),
        ('maxReceiveBuffers','unsignedInt','128'),
        ('maxBlockSize','unsignedInt','131072')
        ]
    foo = Application("foo",1,properties)
    test1 = foo.getProperties()
    XMLtools.indent(test1)
    print( ET.tostring(test1) )

    routing = []
    for id in 1,2,3,4,5:
        routing.append( (
            ('fedid','string',str(id)),
            ('className','string','foobar'),
            ('instance','string',str(id))
            ) )
    properties2 = [
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
        ]
    bar = Application("bar",2,properties2)
    test2 = bar.getProperties()
    XMLtools.indent(test2)
    print( ET.tostring(test2) )
