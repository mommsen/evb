import SymbolMap

class Context:

    def __init__(self,instance,params):
        self.hostInfo = None
        self.app = None
        self.hasPtUtcp = False
        self._id = 11
        self.instance = instance
        self._params = params


    def getConfig(self):
        config = self.getConfigForContext()
        config += self.getConfigForPtUtcp()
        config += self.getConfigForApplication()

        config += """

    <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
    <xc:Module>$XDAQ_ROOT/lib/libptutcp.so</xc:Module>
    <xc:Module>$XDAQ_ROOT/lib/libxdaq2rc.so</xc:Module>
    <xc:Module>$XDAQ_LOCAL/lib/libevb.so</xc:Module>

  </xc:Context>
"""
        return config


    def getTarget(self):
        return """    <i2o:target class="%(app)s" instance="%(instance)s" tid="%(tid)s"/>
""" % {'app':self.app,'instance':self.instance,'tid':self._tid}


    def getConfigForContext(self):
        return """
  <xc:Context url="http://%(soapHostname)s:%(soapPort)s">

    <xc:Endpoint protocol="atcp" service="i2o" hostname="%(i2oHostname)s" port="%(i2oPort)s" network="tcp" targetId="12"/>
""" % self.hostInfo


    def getConfigForPtUtcp(self):
        config = """
    <xc:Application class="pt::utcp::Application" id="%(id)s" instance="0" network="tcp">
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
""" % {'id':self._id}
        self._id += 1
        return config


    def getConfigForApplication(self):
        config = """
     <xc:Application class="%(app)s" id="%(id)s" instance="%(instance)s" network="tcp">
       <properties xmlns="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">
""" % {'app':self.app,'id':self._id,'instance':self.instance}

        self._id += 1
        config += self.fillProperties()
        return config


    def fillProperties(self):
        config = ""
        for param in self._params:
            if isinstance(param[2],tuple):
                config += "         <"+param[0]+" soapenc:arrayType=\"xsd:ur-type["+str(len(param[2]))+"]\" xsi:type=\"soapenc:Array\">\n"
                for index,value in enumerate(param[2]):
                    config += "           <item soapenc:position=\"["+str(index)+"]\" xsi:type=\"xsd:"+param[1]+"\">"+str(value)+"</item>\n"
                config += "         </"+param[0]+">\n"
            else:
                config += "         <"+param[0]+" xsi:type=\"xsd:"+param[1]+"\">"+str(param[2])+"</"+param[0]+">\n"
        config += "       </properties>\n    </xc:Application>"
        return config



class RU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,RU.instance,params)
        RU.instance += 1
        self.hostInfo = symbolMap.getHostInfo('RU'+str(self.instance))
        self._tid = self.instance+1
        if self.instance == 0:
            self.app = "evb::EVM"
        else:
            self.app = "evb::RU"
        self.hasPtUtcp = True



class BU(Context):
    instance = 0

    def __init__(self,symbolMap,params):
        Context.__init__(self,BU.instance,params)
        BU.instance += 1
        self.hostInfo = symbolMap.getHostInfo('BU'+str(self.instance))
        self._tid = self.instance+30
        self.app = "evb::BU"
        self.hasPtUtcp = True



class Configuration():

    def __init__(self):
        self.contexts = []
        self.ptUtcp = []
        self.applications = {}


    def add(self,context):
        self.contexts.append(context)
        appInfo = dict((key,context.hostInfo[key]) for key in ('soapHostname','soapPort'))
        appInfo['instance'] = context.instance
        if context.hasPtUtcp:
            self.ptUtcp.append(appInfo)
        if context.app not in self.applications:
            self.applications[context.app] = []
        self.applications[context.app].append(appInfo)


    def getPartition(self):
        partition = """
    <xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

      <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
    """

        for context in self.contexts:
            partition += context.getTarget()

        partition += "  </i2o:protocol>\n"

        for context in self.contexts:
            partition += context.getConfig()

        partition += "</xc:Partition>"

        return partition
