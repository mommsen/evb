import socket
import ConfigParser

import XDAQprocess


class FedKitConfig:

    def __init__(self):
        self.xdaqProcesses = []
        self.configFilePath = "/tmp/fedKit.xml"
        self._config = ConfigParser.RawConfigParser()
        self._config.read('fedKit.cfg')

        fedId = self.getFedId()
        soapBasePort = self.getSoapBasePort()
        frlBasePort = self.getFrlBasePort()

        with open('fedKit.cfg','wb') as configFile:
            self._config.write(configFile)

        self._xmlConfiguration = self.getConfiguration(fedId,soapBasePort,frlBasePort)

        with open(self.configFilePath,'wb') as xmlFile:
            xmlFile.write(self._xmlConfiguration)


    def getFerolControllerContext(self):
        return ""


    def getDummyFerolContext(self,host,soapPort,frlPort,fedId):
        xdaqProcess = XDAQprocess.XDAQprocess(host,soapPort)
        xdaqProcess.addApplication("pt::frl::Application",0)
        xdaqProcess.addApplication("evb::test::DummyFEROL",0)
        self.xdaqProcesses.append(xdaqProcess)

        return """
      <xc:Context url="http://%(host)s:%(soapPort)s">

        <xc:Endpoint protocol="ftcp" service="frl" hostname="%(host)s" port="%(frlPort)s" network="ferol00" sndTimeout="0" rcvTimeout="2000" affinity="RCV:W,SND:W,DSR:W,DSS:W" singleThread="true" pollingCycle="1" smode="poll"/>

        <xc:Application class="pt::frl::Application" id="11" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
            <ioQueueSize xsi:type="xsd:unsignedInt">32768</ioQueueSize>
            <eventQueueSize xsi:type="xsd:unsignedInt">32768</eventQueueSize>
          </properties>
        </xc:Application>

        <xc:Application class="evb::test::DummyFEROL" id="10" instance="0" network="ferol00">
          <xc:Unicast class="evb::EVM" instance="0" network="ferol00" />
          <properties xmlns="urn:xdaq-application:evb::test::DummyFEROL" xsi:type="soapenc:Struct">
            <fedId xsi:type="xsd:unsignedInt">%(fedId)s</fedId>
            <destinationClass xsi:type="xsd:string">evb::EVM</destinationClass>
            <destinationInstance xsi:type="xsd:unsignedInt">0</destinationInstance>
          </properties>
        </xc:Application>

        <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libptfrl.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libxmemprobe.so</xc:Module>
        <xc:Module>$XDAQ_LOCAL/lib/libevb.so</xc:Module>

      </xc:Context>
    """ % {'host':host,'soapPort':soapPort,'frlPort':frlPort,'fedId':fedId}


    def getEvBContext(self,host,soapPort,frlPort,fedId,useBU):
        xdaqProcess = XDAQprocess.XDAQprocess(host,soapPort)
        #xdaqProcess.addApplication("pt::frl::Application",1)
        xdaqProcess.addApplication("evb::EVM",0)
        if useBU:
            xdaqProcess.addApplication("evb::BU",0)
        self.xdaqProcesses.append(xdaqProcess)

        if useBU:
            dropInputData = "false"
            buApplication = """
        <xc:Application class="evb::BU" id="13" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:evb::BU" xsi:type="soapenc:Struct">
            <dropEventData xsi:type="xsd:boolean">true</dropEventData>
            <numberOfBuilders xsi:type="xsd:unsignedInt">1</numberOfBuilders>
          </properties>
        </xc:Application>
    """
            buTarget = """<i2o:target class="evb::BU" instance="0" tid="30"/>"""
        else:
            dropInputData = "true"
            buApplication = ""
            buTarget = ""

        context = """
      <xc:Context url="http://%(host)s:%(soapPort)s">

        <xc:Endpoint protocol="ftcp" service="frl" hostname="%(host)s" port="%(frlPort)s" network="ferola" sndTimeout="2000" rcvTimeout="0" targetId="12" singleThread="true" pollingCycle="4" rmode="select" nonblock="true" datagramSize="131072" />

        <xc:Alias name="ferol00">ferola</xc:Alias>

        <xc:Application class="pt::frl::Application" id="10" instance="1" network="local">
          <properties xmlns="urn:xdaq-application:pt::frl::Application" xsi:type="soapenc:Struct">
            <frlRouting xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[1]">
              <item xsi:type="soapenc:Struct" soapenc:position="[0]">
                <fedid xsi:type="xsd:string">%(fedId)s</fedid>
                <className xsi:type="xsd:string">evb::EVM</className>
                <instance xsi:type="xsd:string">0</instance>
              </item>
            </frlRouting>
            <frlDispatcher xsi:type="xsd:string">copy</frlDispatcher>
            <useUdaplPool xsi:type="xsd:boolean">true</useUdaplPool>
            <autoConnect xsi:type="xsd:boolean">false</autoConnect>
            <!-- Copy worker configuration -->
            <i2oFragmentBlockSize xsi:type="xsd:unsignedInt">32768</i2oFragmentBlockSize>
            <i2oFragmentsNo xsi:type="xsd:unsignedInt">128</i2oFragmentsNo>
            <i2oFragmentPoolSize xsi:type="xsd:unsignedInt">10000000</i2oFragmentPoolSize>
            <copyWorkerQueueSize xsi:type="xsd:unsignedInt">16</copyWorkerQueueSize>
            <copyWorkersNo xsi:type="xsd:unsignedInt">1</copyWorkersNo>
            <!-- Super fragment configuration -->
            <doSuperFragment xsi:type="xsd:boolean">false</doSuperFragment>
            <!-- Input configuration e.g. PSP -->
            <inputStreamPoolSize xsi:type="xsd:double">1400000</inputStreamPoolSize>
            <maxClients xsi:type="xsd:unsignedInt">5</maxClients>
            <ioQueueSize xsi:type="xsd:unsignedInt">64</ioQueueSize>
            <eventQueueSize xsi:type="xsd:unsignedInt">64</eventQueueSize>
            <maxInputReceiveBuffers xsi:type="xsd:unsignedInt">8</maxInputReceiveBuffers>
            <maxInputBlockSize xsi:type="xsd:unsignedInt">131072</maxInputBlockSize>
          </properties>
        </xc:Application>

        <xc:Application class="evb::EVM" id="12" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:evb::EVM" xsi:type="soapenc:Struct">
            <inputSource xsi:type="xsd:string">FEROL</inputSource>
            <dropInputData xsi:type="xsd:boolean">%(dropInputData)s</dropInputData>
            <numberOfResponders xsi:type="xsd:unsignedInt">1</numberOfResponders>
            <fedSourceIds soapenc:arrayType="xsd:ur-type[1]" xsi:type="soapenc:Array">
              <item soapenc:position="[0]" xsi:type="xsd:unsignedInt">%(fedId)s</item>
            </fedSourceIds>
          </properties>
        </xc:Application>

        %(buApplication)s

        <xc:Module>/usr/lib64/libdat2.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libptfrl.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libptutcp.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libxdaq2rc.so</xc:Module>
        <xc:Module>$XDAQ_LOCAL/lib/libevb.so</xc:Module>

      </xc:Context>

      <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
        <i2o:target class="evb::EVM" instance="0" tid="1"/>
        %(buTarget)s
      </i2o:protocol>
    """ % {'host':host,'soapPort':soapPort,'frlPort':frlPort,'fedId':fedId,'dropInputData':dropInputData,
           'buApplication':buApplication,'buTarget':buTarget}
        return context


    def getConfiguration(self,fedId,soapBasePort,frlBasePort,useBU=True,useDummyFEROL=True):
        hostname = socket.gethostbyaddr(socket.gethostname())[0]

        config = """<xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">"""

        if useDummyFEROL:
            config += self.getDummyFerolContext(hostname,soapBasePort,frlBasePort,fedId);
        else:
            config += self.getFerolControllerContext()

        config += self.getEvBContext(hostname,soapBasePort+1,frlBasePort+1,fedId,useBU)

        config += "</xc:Partition>"

        return config


    def askUserForFedId(self,defaultFedId=None):
        question = "Please enter the FED id you want to read out"
        if defaultFedId is not None:
            question += " ["+str(defaultFedId)+"]: "
        else:
            question += ": "
        answer = raw_input(question)
        answer = answer or defaultFedId
        try:
            fedId=int(answer)
        except (ValueError,NameError,TypeError):
            fedId=-1

        while fedId < 0 or fedId >= 1024:
            try:
                fedId=int(raw_input("Please enter a valid FED id [0..1023]: "))
            except (ValueError,NameError,TypeError):
                fedId=-1

        return fedId


    def getFedId(self):
        fedId = None
        try:
            fedId = self._config.getint('Input','fedId')
        except ConfigParser.NoSectionError:
            self._config.add_section('Input')

        fedId = self.askUserForFedId(fedId)
        self._config.set('Input','fedId',fedId)

        return fedId


    def getSoapBasePort(self):
        try:
            soapBasePort = self._config.getint('XDAQ','soapBasePort')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
            soapBasePort = 65400
            self._config.set('XDAQ','soapBasePort',soapBasePort)

        return soapBasePort


    def getFrlBasePort(self):
        try:
            frlBasePort = self._config.getint('XDAQ','frlBasePort')
        except ConfigParser.NoOptionError:
            frlBasePort = 55300
            self._config.set('XDAQ','frlBasePort',frlBasePort)

        return frlBasePort


if __name__ == "__main__":
    config = FedKitConfig()
    print(config._xmlConfiguration)

