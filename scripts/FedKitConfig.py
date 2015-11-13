import socket
import os
import ConfigParser

import XDAQprocess


class FedKitConfig:

    def __init__(self,configFile,fullConfig,useDummyFerol):
        self.xdaqProcesses = []
        self.configFilePath = "/tmp/fedKit_"+str(os.getpid())+".xml"
        self._config = ConfigParser.ConfigParser()
        self._config.optionxform=str
        self._config.read(configFile)
        self._fullConfig = fullConfig

        with open("/proc/sys/net/ipv4/ip_local_port_range") as portRange:
            self._validPortsMin,self._validPortsMax = [int(x) for x in portRange.readline().split()]

        dataSource = self.getDataSource()
        fedId = self.getFedId()
        writeData = self.getWriteData()
        runNumber = self.getRunNumber()
        lumiSectionDuration = self.getLumiSectionDuration()
        xdaqPort = self.getXdaqPort()
        ferolSourceIP = self.getFerolSourceIP()
        ferolSourcePort = self.getFerolSourcePort()
        ferolDestIP = self.getFerolDestIP()
        ferolDestPort = self.getFerolDestPort()

        with open(configFile,'wb') as configFile:
            self._config.write(configFile)

        self._xmlConfiguration = self.getConfiguration(runNumber,dataSource,fedId,xdaqPort,ferolSourceIP,ferolSourcePort,ferolDestIP,ferolDestPort,writeData,lumiSectionDuration,useDummyFerol)
        #print(self._xmlConfiguration)
        with open(self.configFilePath,'wb') as xmlFile:
            xmlFile.write(self._xmlConfiguration)


    def __del__(self):
        os.remove(self.configFilePath)


    def getFerolControllerContext(self,xdaqHost,xdaqPort,dataSource,ferolDestIP,ferolDestPort,fedId,ferolSourceIP,ferolSourcePort):
        xdaqProcess = XDAQprocess.XDAQprocess(xdaqHost,xdaqPort)
        xdaqProcess.addApplication("ferol::FerolController",0)
        self.xdaqProcesses.append(xdaqProcess)
        if dataSource == 'SLINK_SOURCE':
            operationMode = 'FRL_MODE'
        else:
            operationMode = 'FEDKIT_MODE'

        return """
      <xc:Context url="http://%(xdaqHost)s:%(xdaqPort)s">
        <xc:Application class="ferol::FerolController" id="11" instance="0" network="local" service="ferolcontroller">
          <properties xmlns="urn:xdaq-application:ferol::FerolController" xsi:type="soapenc:Struct">
            <slotNumber xsi:type="xsd:unsignedInt">0</slotNumber>
            <expectedFedId_0 xsi:type="xsd:unsignedInt">%(fedId)s</expectedFedId_0>
            <expectedFedId_1 xsi:type="xsd:unsignedInt">1</expectedFedId_1>
            <SourceIP xsi:type="xsd:string">%(ferolSourceIP)s</SourceIP>
            <TCP_SOURCE_PORT_FED0 xsi:type="xsd:unsignedInt">%(ferolSourcePort)s</TCP_SOURCE_PORT_FED0>
            <TCP_SOURCE_PORT_FED1 xsi:type="xsd:unsignedInt">%(ferolSourcePort)s</TCP_SOURCE_PORT_FED1>
            <enableStream0 xsi:type="xsd:boolean">true</enableStream0>
            <enableStream1 xsi:type="xsd:boolean">false</enableStream1>
            <OperationMode xsi:type="xsd:string">%(operationMode)s</OperationMode>
            <DataSource xsi:type="xsd:string">%(dataSource)s</DataSource>
            <FrlTriggerMode xsi:type="xsd:string">FRL_AUTO_TRIGGER_MODE</FrlTriggerMode>
            <DestinationIP xsi:type="xsd:string">%(ferolDestIP)s</DestinationIP>
            <TCP_DESTINATION_PORT_FED0 xsi:type="xsd:unsignedInt">%(ferolDestPort)s</TCP_DESTINATION_PORT_FED0>
            <TCP_DESTINATION_PORT_FED1 xsi:type="xsd:unsignedInt">%(ferolDestPort)s</TCP_DESTINATION_PORT_FED1>
            <N_Descriptors_FRL xsi:type="xsd:unsignedInt">8192</N_Descriptors_FRL>
            <Seed_FED0 xsi:type="xsd:unsignedInt">14462</Seed_FED0>
            <Seed_FED1 xsi:type="xsd:unsignedInt">14463</Seed_FED1>
            <Event_Length_bytes_FED0 xsi:type="xsd:unsignedInt">2048</Event_Length_bytes_FED0>
            <Event_Length_bytes_FED1 xsi:type="xsd:unsignedInt">2048</Event_Length_bytes_FED1>
            <Event_Length_Stdev_bytes_FED0 xsi:type="xsd:unsignedInt">0</Event_Length_Stdev_bytes_FED0>
            <Event_Length_Stdev_bytes_FED1 xsi:type="xsd:unsignedInt">0</Event_Length_Stdev_bytes_FED1>
            <Event_Length_Max_bytes_FED0 xsi:type="xsd:unsignedInt">65000</Event_Length_Max_bytes_FED0>
            <Event_Length_Max_bytes_FED1 xsi:type="xsd:unsignedInt">65000</Event_Length_Max_bytes_FED1>
            <Event_Delay_ns_FED0 xsi:type="xsd:unsignedInt">20</Event_Delay_ns_FED0>
            <Event_Delay_ns_FED1 xsi:type="xsd:unsignedInt">20</Event_Delay_ns_FED1>
            <Event_Delay_Stdev_ns_FED0 xsi:type="xsd:unsignedInt">0</Event_Delay_Stdev_ns_FED0>
            <Event_Delay_Stdev_ns_FED1 xsi:type="xsd:unsignedInt">0</Event_Delay_Stdev_ns_FED1>
            <TCP_CWND_FED0 xsi:type="xsd:unsignedInt">80000</TCP_CWND_FED0>
            <TCP_CWND_FED1 xsi:type="xsd:unsignedInt">80000</TCP_CWND_FED1>
            <ENA_PAUSE_FRAME xsi:type="xsd:boolean">true</ENA_PAUSE_FRAME>
            <MAX_ARP_Tries xsi:type="xsd:unsignedInt">20</MAX_ARP_Tries>
            <ARP_Timeout_Ms xsi:type="xsd:unsignedInt">1500</ARP_Timeout_Ms>
            <Connection_Timeout_Ms xsi:type="xsd:unsignedInt">7000</Connection_Timeout_Ms>
            <enableSpy xsi:type="xsd:boolean">false</enableSpy>
            <deltaTMonMs xsi:type="xsd:unsignedInt">1000</deltaTMonMs>
            <TCP_CONFIGURATION_FED0 xsi:type="xsd:unsignedInt">16384</TCP_CONFIGURATION_FED0>
            <TCP_CONFIGURATION_FED1 xsi:type="xsd:unsignedInt">16384</TCP_CONFIGURATION_FED1>
            <TCP_OPTIONS_MSS_SCALE_FED0 xsi:type="xsd:unsignedInt">74496</TCP_OPTIONS_MSS_SCALE_FED0>
            <TCP_OPTIONS_MSS_SCALE_FED1 xsi:type="xsd:unsignedInt">74496</TCP_OPTIONS_MSS_SCALE_FED1>
            <TCP_TIMER_RTT_FED0 xsi:type="xsd:unsignedInt">312500</TCP_TIMER_RTT_FED0>
            <TCP_TIMER_RTT_FED1 xsi:type="xsd:unsignedInt">312500</TCP_TIMER_RTT_FED1>
            <TCP_TIMER_RTT_SYN_FED0 xsi:type="xsd:unsignedInt">312500000</TCP_TIMER_RTT_SYN_FED0>
            <TCP_TIMER_RTT_SYN_FED1 xsi:type="xsd:unsignedInt">312500000</TCP_TIMER_RTT_SYN_FED1>
            <TCP_TIMER_PERSIST_FED0 xsi:type="xsd:unsignedInt">40000</TCP_TIMER_PERSIST_FED0>
            <TCP_TIMER_PERSIST_FED1 xsi:type="xsd:unsignedInt">40000</TCP_TIMER_PERSIST_FED1>
            <TCP_REXMTTHRESH_FED0 xsi:type="xsd:unsignedInt">3</TCP_REXMTTHRESH_FED0>
            <TCP_REXMTTHRESH_FED1 xsi:type="xsd:unsignedInt">3</TCP_REXMTTHRESH_FED1>
            <TCP_REXMTCWND_SHIFT_FED0 xsi:type="xsd:unsignedInt">6</TCP_REXMTCWND_SHIFT_FED0>
            <TCP_REXMTCWND_SHIFT_FED1 xsi:type="xsd:unsignedInt">6</TCP_REXMTCWND_SHIFT_FED1>
            <!--N_Descriptors_FED0 xsi:type="xsd:unsignedInt">4</N_Descriptors_FED0>
            <TCP_SOCKET_BUFFER_DDR xsi:type="xsd:boolean">false</TCP_SOCKET_BUFFER_DDR>
            <DDR_memory_mask xsi:type="xsd:unsignedInt">0x0fffffff</DDR_memory_mask>
            <QDR_memory_mask xsi:type="xsd:unsignedInt">0x007fffff</QDR_memory_mask>
            <lightStop xsi:type="xsd:boolean">false</lightStop-->
          </properties>
        </xc:Application>

        <xc:Module>$XDAQ_ROOT/lib/libFerolController.so</xc:Module>

      </xc:Context>
        """ % {'xdaqHost':xdaqHost,'xdaqPort':xdaqPort,'dataSource':dataSource,'operationMode':operationMode,'ferolDestIP':ferolDestIP,'ferolDestPort':ferolDestPort,'fedId':fedId,'ferolSourceIP':ferolSourceIP,'ferolSourcePort':ferolSourcePort}


    def getDummyFerolContext(self,xdaqHost,xdaqPort,ferolSourceIP,ferolSourcePort,ferolDestIP,ferolDestPort,fedId):
        xdaqProcess = XDAQprocess.XDAQprocess(xdaqHost,xdaqPort)
        xdaqProcess.addApplication("evb::test::DummyFEROL",0)
        self.xdaqProcesses.append(xdaqProcess)

        return """
      <xc:Context url="http://%(xdaqHost)s:%(xdaqPort)s">

        <xc:Application class="evb::test::DummyFEROL" id="10" instance="0" network="local">
          <xc:Unicast class="evb::EVM" instance="0" network="ferol00" />
          <properties xmlns="urn:xdaq-application:evb::test::DummyFEROL" xsi:type="soapenc:Struct">
            <fedId xsi:type="xsd:unsignedInt">%(fedId)s</fedId>
            <sourceHost xsi:type="xsd:string">%(ferolSourceIP)s</sourceHost>
            <sourcePort xsi:type="xsd:unsignedInt">%(ferolSourcePort)s</sourcePort>
            <destinationHost xsi:type="xsd:string">%(ferolDestIP)s</destinationHost>
            <destinationPort xsi:type="xsd:unsignedInt">%(ferolDestPort)s</destinationPort>
          </properties>
        </xc:Application>

        <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libevb.so</xc:Module>

      </xc:Context>
        """ % {'xdaqHost':xdaqHost,'xdaqPort':xdaqPort,'fedId':fedId,'ferolSourceIP':ferolSourceIP,'ferolSourcePort':ferolSourcePort,'ferolDestIP':ferolDestIP,'ferolDestPort':ferolDestPort}


    def getEvBContext(self,xdaqHost,xdaqPort,ferolDestIP,ferolDestPort,ferolSourceIP,ferolSourcePort,runNumber,fedId,writeData,lumiSectionDuration):
        xdaqProcess = XDAQprocess.XDAQprocess(xdaqHost,xdaqPort)
        xdaqProcess.addApplication("evb::EVM",0)
        xdaqProcess.addApplication("evb::BU",0)
        self.xdaqProcesses.append(xdaqProcess)

        if writeData:
            dropEventData = "false"
            outputDir = self.getOutputDir()
            fakeLumiSectionDuration = lumiSectionDuration
            lumiSectionTimeout = str(int(lumiSectionDuration)+10)
        else:
            dropEventData = "true"
            outputDir = "/tmp"
            fakeLumiSectionDuration = 0
            lumiSectionTimeout = 0

        context = """
      <xc:Context url="http://%(xdaqHost)s:%(xdaqPort)s">

         <xc:Endpoint affinity="RCV:P,SND:W,DSR:W,DSS:W" hostname="%(ferolDestIP)s" maxbulksize="131072" network="ferol" nonblock="true" pollingCycle="1" port="%(ferolDestPort)s" protocol="btcp" rcvTimeout="0" rmode="select" service="blit" singleThread="true" sndTimeout="2000" targetId="11" />

        <xc:Application class="pt::blit::Application" id="11" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:evb::EVM" xsi:type="soapenc:Struct">
            <maxClients xsi:type="xsd:unsignedInt">5</maxClients>
            <maxReceiveBuffers xsi:type="xsd:unsignedInt">16</maxReceiveBuffers>
            <maxBlockSize xsi:type="xsd:unsignedInt">131072</maxBlockSize>
          </properties>
        </xc:Application>

        <xc:Application class="evb::EVM" id="12" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:evb::EVM" xsi:type="soapenc:Struct">
            <inputSource xsi:type="xsd:string">Socket</inputSource>
            <runNumber xsi:type="xsd:unsignedInt">%(runNumber)s</runNumber>
            <fakeLumiSectionDuration xsi:type="xsd:unsignedInt">%(fakeLumiSectionDuration)s</fakeLumiSectionDuration>
            <numberOfResponders xsi:type="xsd:unsignedInt">1</numberOfResponders>
            <checkCRC xsi:type="xsd:unsignedInt">1</checkCRC>
            <fedSourceIds soapenc:arrayType="xsd:ur-type[1]" xsi:type="soapenc:Array">
              <item soapenc:position="[0]" xsi:type="xsd:unsignedInt">%(fedId)s</item>
            </fedSourceIds>
            <ferolSources soapenc:arrayType="xsd:ur-type[1]" xsi:type="soapenc:Array">
              <item soapenc:position="[0]" xsi:type="soapenc:Struct">
                <fedId xsi:type="xsd:unsignedInt">%(fedId)s</fedId>
                <hostname xsi:type="xsd:string">%(ferolSourceIP)s</hostname>
                <port xsi:type="xsd:unsignedInt">%(ferolSourcePort)s</port>
              </item>
            </ferolSources>
          </properties>
        </xc:Application>

        <xc:Application class="evb::BU" id="13" instance="0" network="local">
          <properties xmlns="urn:xdaq-application:evb::BU" xsi:type="soapenc:Struct">
            <runNumber xsi:type="xsd:unsignedInt">%(runNumber)s</runNumber>
            <dropEventData xsi:type="xsd:boolean">%(dropEventData)s</dropEventData>
            <ignoreResourceSummary xsi:type="xsd:boolean">true</ignoreResourceSummary>
            <lumiSectionTimeout xsi:type="xsd:unsignedInt">%(lumiSectionTimeout)s</lumiSectionTimeout>
            <numberOfBuilders xsi:type="xsd:unsignedInt">1</numberOfBuilders>
            <checkCRC xsi:type="xsd:unsignedInt">1</checkCRC>
            <rawDataDir xsi:type="xsd:string">%(outputDir)s</rawDataDir>
            <metaDataDir xsi:type="xsd:string">%(outputDir)s</metaDataDir>
            <rawDataHighWaterMark xsi:type="xsd:double">0.99</rawDataHighWaterMark>
            <rawDataLowWaterMark xsi:type="xsd:double">0.9</rawDataLowWaterMark>
          </properties>
        </xc:Application>

        <xc:Module>$XDAQ_ROOT/lib/libtcpla.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libptblit.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libptutcp.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libxdaq2rc.so</xc:Module>
        <xc:Module>$XDAQ_ROOT/lib/libevb.so</xc:Module>

      </xc:Context>

      <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
        <i2o:target class="evb::EVM" instance="0" tid="1"/>
        <i2o:target class="evb::BU" instance="0" tid="30"/>
    </i2o:protocol>
        """ % {'xdaqHost':xdaqHost,'xdaqPort':xdaqPort,'ferolDestIP':ferolDestIP,'ferolDestPort':ferolDestPort,'ferolSourceIP':ferolSourceIP,'ferolSourcePort':ferolSourcePort,
               'runNumber':runNumber,'fedId':fedId,
               'dropEventData':dropEventData,'outputDir':outputDir,'fakeLumiSectionDuration':fakeLumiSectionDuration,'lumiSectionTimeout':lumiSectionTimeout}
        return context


    def getConfiguration(self,runNumber,dataSource,fedId,xdaqPort,ferolSourceIP,ferolSourcePort,ferolDestIP,ferolDestPort,writeData,lumiSectionDuration,useDummyFEROL):
        hostname = socket.gethostbyaddr(socket.gethostname())[0]

        config = """<xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">"""

        if useDummyFEROL:
            config += self.getDummyFerolContext(hostname,xdaqPort,ferolSourceIP,ferolSourcePort,ferolDestIP,ferolDestPort,fedId);
        else:
            config += self.getFerolControllerContext(hostname,xdaqPort,dataSource,ferolDestIP,ferolDestPort,fedId,ferolSourceIP,ferolSourcePort)

        config += self.getEvBContext(hostname,xdaqPort+1,ferolDestIP,ferolDestPort,ferolSourceIP,ferolSourcePort,runNumber,fedId,writeData,lumiSectionDuration)

        config += "</xc:Partition>"

        return config


    def askYesNo(self,question,default=None):
        if default is True:
            question += " [Yes]"
        elif default is False:
            question += " [No]"
        question += ":"

        answer = None
        while not answer:
            answer = raw_input(question)
            if not answer and default is not None:
                return default
            elif answer.lower()[0] == 'y':
                return True
            elif answer.lower()[0] == 'n':
                return False


    def getDataSource(self):
        dataSource = None
        possibleDataSources = ('L6G_SOURCE','L6G_CORE_GENERATOR_SOURCE','L6G_LOOPBACK_GENERATOR_SOURCE','L10G_SOURCE','L10G_CORE_GENERATOR_SOURCE','SLINK_SOURCE')

        try:
            dataSource = self._config.get('Input','dataSource')
        except ConfigParser.NoSectionError:
            self._config.add_section('Input')
        except ConfigParser.NoOptionError:
            pass

        print("""Please select the data source to be used:
  1 - AMC13 data source over 6Gb        (L6G_SOURCE)
  2 - Generator core of AMC13 over 6Gb  (L6G_CORE_GENERATOR_SOURCE)
  3 - Loopback at the FEROL             (L6G_LOOPBACK_GENERATOR_SOURCE)
  4 - AMC13 data source over 10Gb       (L10G_SOURCE)
  5 - Generator core of AMC13 over 10Gb (L10G_CORE_GENERATOR_SOURCE)
  6 - SLINK data source                 (SLINK_SOURCE)""")

        prompt = "=> "
        try:
            default = possibleDataSources.index(dataSource) + 1
            prompt += '['+str(default)+'] '
        except ValueError:
            default = 0

        chosenDataSource = None
        while chosenDataSource is None:
            answer = raw_input(prompt)
            answer = answer or default
            try:
                index = int(answer) - 1
                if index < 0:
                    raise ValueError
                chosenDataSource = possibleDataSources[index]
            except (ValueError,NameError,TypeError,IndexError):
                print("Please enter a valid choice [1.."+str(len(possibleDataSources))+"]")

        dataSource = chosenDataSource
        self._config.set('Input','dataSource',dataSource)

        return dataSource


    def askUserForFedId(self,defaultFedId):
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

        while fedId < 0 or fedId >= 1350:
            try:
                fedId=int(raw_input("Please enter a valid FED id [0..1350]: "))
            except (ValueError,NameError,TypeError):
                fedId=-1

        return fedId


    def getFedId(self):
        fedId = None
        try:
            fedId = self._config.getint('Input','fedId')
        except ConfigParser.NoSectionError:
            self._config.add_section('Input')
        except ConfigParser.NoOptionError:
            pass

        fedId = self.askUserForFedId(fedId)
        self._config.set('Input','fedId',fedId)

        return fedId


    def askUserForRunNumber(self,defaultRunNumber):
        question = "Please enter the run number ["+str(defaultRunNumber)+"]: "
        answer = raw_input(question)
        answer = answer or defaultRunNumber
        try:
            runNumber=int(answer)
        except (ValueError,NameError,TypeError):
            runNumber=-1

        while runNumber < 0:
            try:
                runNumber=int(raw_input("Please enter a positive integer as run number: "))
            except (ValueError,NameError,TypeError):
                runNumber=-1

        return runNumber


    def getRunNumber(self):
        runNumber = 1
        try:
            runNumber = self._config.getint('Input','runNumber') + 1
        except ConfigParser.NoSectionError:
            self._config.add_section('Input')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            runNumber = self.askUserForRunNumber(runNumber)

        self._config.set('Input','runNumber',runNumber)

        return runNumber


    def askUserForLumiSectionDuration(self,defaultLumiSectionDuration):
        question = "Please enter the duration of the fake lumisection (s) ["+str(defaultLumiSectionDuration)+"]: "
        answer = raw_input(question)
        answer = answer or defaultLumiSectionDuration
        try:
            lumiSectionDuration=int(answer)
        except (ValueError,NameError,TypeError):
            lumiSectionDuration=-1

        while lumiSectionDuration < 0:
            try:
                lumiSectionDuration=int(raw_input("Please enter a positive integer as lumi section duration: "))
            except (ValueError,NameError,TypeError):
                lumiSectionDuration=-1

        return lumiSectionDuration


    def getLumiSectionDuration(self):
        lumiSectionDuration = 23
        try:
            runNumber = self._config.getint('Input','lumiSectionDuration')
        except ConfigParser.NoSectionError:
            self._config.add_section('Input')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            lumiSectionDuration = self.askUserForLumiSectionDuration(lumiSectionDuration)

        self._config.set('Input','lumiSectionDuration',lumiSectionDuration)

        return lumiSectionDuration


    def getWriteData(self):
        writeData = False
        try:
            writeData = self._config.getboolean('Output','writeData')
        except ConfigParser.NoSectionError:
            self._config.add_section('Output')
        except ConfigParser.NoOptionError:
            pass

        writeData = self.askYesNo("Do you want to write the data to disk?",writeData)

        self._config.set('Output','writeData',writeData)

        return writeData


    def getOutputDir(self):
        try:
            outputDir = self._config.get('Output','outputDir')
        except ConfigParser.NoSectionError:
            self._config.add_section('Output')
        except ConfigParser.NoOptionError:
            pass

        outputDir = raw_input("Please enter the directory where the data shall be written [/tmp]: ")
        outputDir = outputDir or '/tmp'
        self._config.set('Output','outputDir',outputDir)

        return outputDir


    def askUserForIP(self,defaultIP,forWhat):
        question = "Please enter the IP address of the "+forWhat+" ["+str(defaultIP)+"]: "
        ip = raw_input(question)
        ip = ip or defaultIP

        return ip


    def askUserForPort(self,defaultPort,forWhat):
        question = "Please enter the port of the "+forWhat+" ["+str(defaultPort)+"]: "
        answer = raw_input(question)
        answer = answer or defaultPort
        try:
            port=int(answer)
        except (ValueError,NameError,TypeError):
            port=-1

        while port < self._validPortsMin or port > self._validPortsMax:
            try:
                port=int(raw_input("Please enter a valid port number ["+str(self._validPortsMin)+".."+str(self._validPortsMax)+"]: "))
            except (ValueError,NameError,TypeError):
                port=-1

        return port


    def getXdaqPort(self):
        xdaqPort = ((self._validPortsMin // 1000) + 1) * 1000
        try:
            xdaqPort = self._config.getint('XDAQ','xdaqPort')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            xdaqPort = self.askUserForPort(xdaqPort,"XDAQ application")

        self._config.set('XDAQ','xdaqPort',xdaqPort)

        return xdaqPort


    def getFerolSourceIP(self):
        ferolSourceIP = "10.0.0.4"
        try:
            ferolSourceIP = self._config.get('XDAQ','ferolSourceIP')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            ferolSourceIP = self.askUserForIP(ferolSourceIP,"FEROL source")

        self._config.set('XDAQ','ferolSourceIP',ferolSourceIP)

        return ferolSourceIP


    def getFerolSourcePort(self):
        ferolSourcePort = ((self._validPortsMin // 1000) + 2) * 1000
        try:
            ferolSourcePort = self._config.get('XDAQ','ferolSourcePort')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            ferolSourcePort = self.askUserForPort(ferolSourcePort,"FEROL source")

        self._config.set('XDAQ','ferolSourcePort',ferolSourcePort)

        return ferolSourcePort


    def getFerolDestIP(self):
        ferolDestIP = "10.0.0.5"
        try:
            ferolDestIP = self._config.get('XDAQ','ferolDestIP')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            ferolDestIP = self.askUserForIP(ferolDestIP,"FEROL destination")

        self._config.set('XDAQ','ferolDestIP',ferolDestIP)

        return ferolDestIP


    def getFerolDestPort(self):
        ferolDestPort = ((self._validPortsMin // 1000) + 2) * 1000
        try:
            ferolDestPort = self._config.getint('XDAQ','ferolDestPort')
        except ConfigParser.NoSectionError:
            self._config.add_section('XDAQ')
        except ConfigParser.NoOptionError:
            pass

        if self._fullConfig:
            ferolDestPort = self.askUserForPort(ferolDestPort,"FEROL destination")

        self._config.set('XDAQ','ferolDestPort',ferolDestPort)

        return ferolDestPort


if __name__ == "__main__":
    config = FedKitConfig("tmp.cfg")
    print(config._xmlConfiguration)
