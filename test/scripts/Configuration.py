import copy

import SymbolMap


class Context:
    ptInstance = 0

    def __init__(self,params):
        self._params = params
        self.role = None
        self.apps = {
            'app':None,
            'instance':None,
            'ptApp':None,
            'ptInstance':Context.ptInstance,
            'tid':None,
            'id':11
            }
        Context.ptInstance += 1
        self._id = 11


    def __del__(self):
        Context.ptInstance = 0


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
        return """      <i2o:target class="%(app)s" instance="%(instance)s" tid="%(tid)s"/>
""" % self.apps


    def getConfigForContext(self):
        return """
    <xc:Context url="http://%(soapHostname)s:%(soapPort)s">
""" % self.apps


    def getConfigForPtFrl(self):
        return ""


    def getConfigForPtUtcp(self):
        if self.apps['ptApp'] != "pt::utcp::Application":
             return ""

        config = """
      <xc:Endpoint protocol="atcp" service="i2o" hostname="%(i2oHostname)s" port="%(i2oPort)s" network="tcp"/>

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
""" % dict(self.apps.items() + [('id',self._id)])
        self._id += 1
        return config


    def getConfigForApplication(self):
        config = """
      <xc:Application class="%(app)s" id="%(id)s" instance="%(instance)s" network="tcp">
         <properties xmlns="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">
""" % dict(self.apps.items() + [('id',self._id)])
        self._id += 1
        config += self.fillProperties(self._params)
        config += "         </properties>\n      </xc:Application>\n"
        return config


    def fillProperties(self,params):
        config = ""
        for param in params:
            if isinstance(param[2],tuple) or isinstance(param[2],list):
                config += "           <"+param[0]+" soapenc:arrayType=\"xsd:ur-type["+str(len(param[2]))+"]\" xsi:type=\"soapenc:Array\">\n"
                for index,value in enumerate(param[2]):
                    if param[1] == 'Struct':
                        config += "             <item soapenc:position=\"["+str(index)+"]\" xsi:type=\"soapenc:Struct\">"
                        for val in value:
                            config += "\n               <"+val[0]+" xsi:type=\"xsd:"+val[1]+"\">"+str(val[2])+"</"+val[0]+">"
                        config += "\n             </item>\n"
                    else:
                        config += "             <item soapenc:position=\"["+str(index)+"]\" xsi:type=\"xsd:"+param[1]+"\">"+str(value)+"</item>\n"
                config += "           </"+param[0]+">\n"
            else:
                config += "           <"+param[0]+" xsi:type=\"xsd:"+param[1]+"\">"+str(param[2])+"</"+param[0]+">\n"
        return config



class FEROL(Context):
    instance = 0

    def __init__(self,symbolMap,destination,params):
        Context.__init__(self,params)
        self.role = "FEROL"
        self.apps['app'] = "evb::test::DummyFEROL"
        self.apps['instance'] = FEROL.instance
        self.apps.update( symbolMap.getHostInfo('FEROL'+str(FEROL.instance)) )
        self.apps['ptApp'] = "pt::frl::Application"
        FEROL.instance += 1

        self._unicast = """<xc:Unicast class="%(destinationApp)s" instance="%(destinationInstance)s" network=\"ferol%(instance)s" />"""\
            % {'destinationApp':destination.apps['app'],'destinationInstance':destination.apps['instance'],'instance':self.apps['instance']}
        self._params.append(('destinationClass','string',destination.apps['app']))
        self._params.append(('destinationInstance','unsignedInt',str(destination.apps['instance'])))


    def __del__(self):
        FEROL.instance = 0


    def getConfigForApplication(self):
        config = """
      <xc:Application class="%(app)s" id="%(id)s" instance="%(instance)s" network="ferol%(instance)s">
        %(unicast)s
        <properties xmlns="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">
""" % dict(self.apps.items() + [('id',self._id),('unicast',self._unicast)])
        self._id += 1
        config += self.fillProperties(self._params)
        config += "        </properties>\n      </xc:Application>\n"
        return config


    def getConfigForPtFrl(self):
        config = """
      <xc:Endpoint protocol="ftcp" service="frl" hostname="%(frlHostname)s" port="%(frlPort)s" network="ferol%(instance)s" sndTimeout="0" rcvTimeout="2000" affinity="RCV:W,SND:W,DSR:W,DSS:W" singleThread="true" pollingCycle="1" smode="poll"/>

      <xc:Application class="pt::frl::Application" id="%(id)s" instance="%(ptInstance)s" network="ferol%(instance)s">
        <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
""" % dict(self.apps.items() + [('id',self._id)])
        self._id += 1
        config += self.fillProperties([
            ('ioQueueSize','unsignedInt','65536'),
            ('eventQueueSize','unsignedInt','65536')
            ])
        config += "        </properties>\n      </xc:Application>\n"
        return config



class RU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,params)
        if RU.instance == 0:
            self.apps['app'] = "evb::EVM"
            self.role = "EVM"
        else:
            self.apps['app'] = "evb::RU"
            self.role = "RU"
        self.apps['instance'] = RU.instance
        self.apps.update( symbolMap.getHostInfo('RU'+str(RU.instance)) )
        self.apps['tid'] = RU.instance+1
        self.apps['ptApp'] = "pt::utcp::Application"
        RU.instance += 1


    def __del__(self):
        RU.instance = 0


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
                            ('className','string',self.apps['app']),
                            ('instance','string',str(self.apps['instance']))
                            ) )
            except KeyError:
                pass

        config = """
      <xc:Endpoint protocol="ftcp" service="frl" hostname="%(frlHostname)s" port="%(frlPort)s" network="ferol" sndTimeout="2000" rcvTimeout="0" singleThread="true" pollingCycle="4" rmode="select" nonblock="true" datagramSize="131072" />

      <xc:Application class="pt::frl::Application" id="%(id)s" instance="%(ptInstance)s" network="ferol">
        <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
"""  % dict(self.apps.items() + [('id',self._id)])
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
        config += "        </properties>\n      </xc:Application>\n"

        for i in range(len(routing)):
            config += "\n      <xc:Alias name=\"ferol"+str(i)+"\">ferol</xc:Alias>"

        config += "\n"
        return config



class BU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,params)
        self.role = "BU"
        self.apps['app'] = "evb::BU"
        self.apps['instance'] = BU.instance
        self.apps.update( symbolMap.getHostInfo('BU'+str(BU.instance)) )
        self.apps['tid'] = BU.instance+30
        self.apps['ptApp'] = "pt::utcp::Application"
        BU.instance += 1


    def __del__(self):
        BU.instance = 0


    def getConfigForPtFrl(self):
        config = """
      <xc:Application class="pt::frl::Application" id="%(id)s" instance="%(ptInstance)s" network="local">
        <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
        </properties>
      </xc:Application>
""" % dict(self.apps.items() + [('id',self._id)])
        self._id += 1
        return config


class Configuration():

    def __init__(self):
        self.contexts = []
        self.pt = []
        self.applications = {}


    def add(self,context):
        self.contexts.append(context)
        appInfo = dict((key,context.apps[key]) for key in ('app','instance','soapHostname','soapPort'))
        if context.role not in self.applications:
            self.applications[context.role] = []
        self.applications[context.role].append(copy.deepcopy(appInfo))
        if context.apps['ptApp'] is not None:
            appInfo['app'] = context.apps['ptApp']
            appInfo['instance'] = context.apps['ptInstance']
            self.pt.append(appInfo)


    def getPartition(self):
        partition = """
  <xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

    <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
"""

        for context in self.contexts:
            if context.apps['tid'] is not None:
                partition += context.getTarget()

        partition += "    </i2o:protocol>\n"

        for context in self.contexts:
            partition += context.getConfig()

        partition += "  </xc:Partition>\n"

        return partition
