import copy

import SymbolMap


class Context:
    ptInstance = 0

    def __init__(self,params):
        self.app = None
        self.appInstance = None
        self.ptInstance = Context.ptInstance
        self.ptApp = None
        self.hostInfo = None
        self._id = 11
        self.tid = None
        Context.ptInstance += 1
        self._params = params


    def getConfig(self):
        config = self.getConfigForContext()
        config += self.getConfigForPtFrl()
        config += self.getConfigForPtUtcp()
        config += self.getConfigForApplication()

        config += """

    <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
    <xc:Module>$XDAQ_ROOT/lib/libptfrl.so</xc:Module>
    <xc:Module>$XDAQ_ROOT/lib/libptutcp.so</xc:Module>
    <xc:Module>$XDAQ_ROOT/lib/libxdaq2rc.so</xc:Module>
    <xc:Module>$XDAQ_LOCAL/lib/libevb.so</xc:Module>

  </xc:Context>
"""
        return config


    def getTarget(self):
        return """    <i2o:target class="%(app)s" instance="%(appInstance)s" tid="%(tid)s"/>
""" % {'app':self.app,'appInstance':self.appInstance,'tid':self.tid}


    def getConfigForContext(self):
        return """
  <xc:Context url="http://%(soapHostname)s:%(soapPort)s">
""" % self.hostInfo


    def getConfigForPtFrl(self):
        return ""


    def getConfigForPtUtcp(self):
        if self.ptApp != "pt::utcp::Application":
             return ""

        config = """
    <xc:Endpoint protocol="atcp" service="i2o" hostname="%(i2oHostname)s" port="%(i2oPort)s" network="tcp"/>
""" % self.hostInfo
        config += """
    <xc:Application class="pt::utcp::Application" id="%(id)s" instance="%(ptInstance)s" network="tcp">
      <properties xmlns="urn:xdaq-application:pt::utcp::Application" xsi:type="soapenc:Struct">
        <protocol xsi:type="xsd:string">atcp</protocol>
        <maxClients xsi:type="xsd:unsignedInt">9</maxClients>
        <autoConnect xsi:type="xsd:boolean">false</autoConnect>
        <ioQueueSize xsi:type="xsd:unsignedInt">65536</ioQueueSize>
        <eventQueueSize xsi:type="xsd:unsignedInt">65536</eventQueueSize>
        <maxReceiveBuffers xsi:type="xsd:unsignedInt">12</maxReceiveBuffers>
        <maxBlockSize xsi:type="xsd:unsignedInt">65537</maxBlockSize>
      </properties>
    </xc:Application>
""" % {'id':self._id,'ptInstance':self.ptInstance}
        self._id += 1
        return config


    def getConfigForApplication(self):
        config = """
     <xc:Application class="%(app)s" id="%(id)s" instance="%(appInstance)s" network="tcp">
       <properties xmlns="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">
""" % {'app':self.app,'id':self._id,'appInstance':self.appInstance}

        self._id += 1
        config += self.fillProperties(self._params)
        config += "       </properties>\n    </xc:Application>"
        return config


    def fillProperties(self,params):
        config = ""
        for param in params:
            if isinstance(param[2],tuple) or isinstance(param[2],list):
                config += "         <"+param[0]+" soapenc:arrayType=\"xsd:ur-type["+str(len(param[2]))+"]\" xsi:type=\"soapenc:Array\">\n"
                for index,value in enumerate(param[2]):
                    if param[1] == 'Struct':
                        config += "           <item soapenc:position=\"["+str(index)+"]\" xsi:type=\"soapenc:Struct\">"
                        for val in value:
                            config += "\n             <"+val[0]+" xsi:type=\"xsd:"+val[1]+"\">"+str(val[2])+"</"+val[0]+">"
                        config += "\n           </item>\n"
                    else:
                        config += "           <item soapenc:position=\"["+str(index)+"]\" xsi:type=\"xsd:"+param[1]+"\">"+str(value)+"</item>\n"
                config += "         </"+param[0]+">\n"
            else:
                config += "         <"+param[0]+" xsi:type=\"xsd:"+param[1]+"\">"+str(param[2])+"</"+param[0]+">\n"
        return config



class FEROL(Context):
    instance = 0

    def __init__(self,symbolMap,destination,params):
        Context.__init__(self,params)
        self.appInstance = FEROL.instance
        FEROL.instance += 1
        self.hostInfo = symbolMap.getHostInfo('FEROL'+str(self.appInstance))
        self.app = "evb::test::DummyFEROL"
        self.ptApp = "pt::frl::Application"
        self._unicast = "<xc:Unicast class=\""+destination.app+"\" instance=\""+str(destination.appInstance)+"\" network=\"ferol\" />"
        self._params.append(('destinationClass','string',destination.app))
        self._params.append(('destinationInstance','unsignedInt',str(destination.appInstance)))


    def getConfigForApplication(self):
        config = """
     <xc:Application class="%(app)s" id="%(id)s" instance="%(appInstance)s" network="ferol">
       %(unicast)s
       <properties xmlns="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">
""" % {'app':self.app,'id':self._id,'appInstance':self.appInstance,'unicast':self._unicast}

        self._id += 1
        config += self.fillProperties(self._params)
        config += "       </properties>\n    </xc:Application>"
        return config


    def getConfigForPtFrl(self):
        config = """
    <xc:Endpoint protocol="ftcp" service="frl" hostname="%(frlHostname)s" port="%(frlPort)s" network="ferol" sndTimeout="0" rcvTimeout="2000" affinity="RCV:W,SND:W,DSR:W,DSS:W" singleThread="true" pollingCycle="1" smode="poll"/>
""" % self.hostInfo
        config += """
    <xc:Application class="pt::frl::Application" id="%(id)s" instance="%(ptInstance)s" network="ferol">
      <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
""" % {'id':self._id,'ptInstance':self.ptInstance}
        self._id += 1
        config += self.fillProperties([
            ('ioQueueSize','unsignedInt','65536'),
            ('eventQueueSize','unsignedInt','65536')
            ])
        config += "       </properties>\n     </xc:Application>\n"
        return config



class RU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,params)
        self.appInstance = RU.instance
        RU.instance += 1
        self.hostInfo = symbolMap.getHostInfo('RU'+str(self.appInstance))
        self.tid = self.appInstance+1
        if self.appInstance == 0:
            self.app = "evb::EVM"
        else:
            self.app = "evb::RU"
        self.ptApp = "pt::utcp::Application"


    def getConfigForPtFrl(self):
        routing = []
        for param in self._params:
            try:
                if param[0] == 'inputSource' and param[2] == 'Local':
                    return ""
                if param[0] == 'fedSourceIds':
                    for fedId in param[2]:
                        routing.append( (
                            ('fedid','string',str(fedId)),
                            ('className','string',self.app),
                            ('instance','string',str(self.appInstance))
                            ) )
            except KeyError:
                pass

        config = """
    <xc:Endpoint protocol="ftcp" service="frl" hostname="%(frlHostname)s" port="%(frlPort)s" network="ferol" sndTimeout="2000" rcvTimeout="0" singleThread="true" pollingCycle="4" rmode="select" nonblock="true" datagramSize="131072" />
""" % self.hostInfo
        config += """
    <xc:Application class="pt::frl::Application" id="%(id)s" instance="%(ptInstance)s" network="ferol">
      <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
""" % {'id':self._id,'ptInstance':self.ptInstance}
        self._id += 1

        config += self.fillProperties([
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
        config += "       </properties>\n     </xc:Application>\n"
        return config



class BU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,params)
        self.appInstance = BU.instance
        BU.instance += 1
        self.hostInfo = symbolMap.getHostInfo('BU'+str(self.appInstance))
        self.tid = self.appInstance+30
        self.app = "evb::BU"
        self.ptApp = "pt::utcp::Application"


class Configuration():

    def __init__(self):
        self.contexts = []
        self.pt = []
        self.applications = {}


    def add(self,context):
        self.contexts.append(context)
        appInfo = dict((key,context.hostInfo[key]) for key in ('soapHostname','soapPort'))
        appInfo['app'] = context.app
        appInfo['instance'] = context.appInstance
        if context.app not in self.applications:
            self.applications[context.app] = []
        self.applications[context.app].append(copy.deepcopy(appInfo))
        if context.ptApp is not None:
            appInfo['app'] = context.ptApp
            appInfo['instance'] = context.ptInstance
            self.pt.append(copy.deepcopy(appInfo))


    def getPartition(self):
        partition = """
<xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

  <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
"""

        for context in self.contexts:
            if context.tid is not None:
                partition += context.getTarget()

        partition += "  </i2o:protocol>\n"

        for context in self.contexts:
            partition += context.getConfig()

        partition += "</xc:Partition>"

        return partition
