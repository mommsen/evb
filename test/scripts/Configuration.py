import copy
import gzip
import os
import re
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Application
import Context
import SymbolMap
import XMLtools

class Configuration():

    def __init__(self,symbolMap,useNuma):
        self.symbolMap = symbolMap
        self.useNuma = useNuma
        self.contexts = {}
        self.ptUtcp = []
        self.ptIBV = []
        self.applications = {}
        self.xcns = 'http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30'


    def add(self,context):
        appInfo = dict((key,context.hostinfo[key]) for key in ('soapHostname','soapPort','launcherPort'))
        contextKey = (appInfo['soapHostname'],appInfo['soapPort'])
        self.contexts[contextKey] = context
        for app in context.applications:
            appInfo['app'] = app.params['class']
            appInfo['instance'] = app.params['instance']
            if app.params['class'].startswith(('evb::test::DummyFEROL','ferol::','ferol40::')):
                self.addAppInfoToApplications('FEROL',appInfo)
            elif app.params['class'] == 'evb::EVM':
                self.addAppInfoToApplications('EVM',appInfo)
                self.evmFedId = self.getEvmFedId(app)
            elif app.params['class'] == 'evb::RU':
                self.addAppInfoToApplications('RU',appInfo)
            elif app.params['class'] == 'evb::BU':
                self.addAppInfoToApplications('BU',appInfo)
            elif app.params['class'] == 'pt::utcp::Application':
                self.ptUtcp.append( copy.deepcopy(appInfo) )
            elif app.params['class'] == 'pt::ibv::Application':
                self.ptIBV.append( copy.deepcopy(appInfo) )


    def getEvmFedId(self,app):
        newProp = []
        for prop in app.properties:
            if prop[0] == 'fedSourceIds':
                return prop[2][0]


    def addAppInfoToApplications(self,role,appInfo):
        if role not in self.applications:
            self.applications[role] = []
        self.applications[role].append( copy.deepcopy(appInfo) )


    def isI2Otarget(self,myKey,otherKey):
        if myKey == otherKey:
            return True
        if self.contexts[myKey].role == 'EVM' and self.contexts[otherKey].role in ('RU','BU','RUBU'):
            return True
        if self.contexts[myKey].role == 'RU' and self.contexts[otherKey].role in ('BU','RUBU'):
            return True
        if self.contexts[myKey].role == 'BU' and self.contexts[otherKey].role == 'EVM':
            return True
        if self.contexts[myKey].role == 'RUBU' and self.contexts[otherKey].role in ('EVM','BU','RUBU'):
            return True
        return False


    def getTargets(self,key):
        myRole = self.contexts[key].role
        i2ons = "http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30"
        protocol = ET.Element(QN(i2ons,'protocol'))
        for k,c in self.contexts.items():
            if self.isI2Otarget(key,k):
                for app in c.applications:
                    app.addTargetElement(protocol,i2ons)
        return protocol


    def getPartition(self,key):

        partition = ET.Element(QN(self.xcns,'Partition'))

        protocol = self.getTargets(key)
        if len(protocol) > 0:
            partition.append(protocol)

        for k,c in self.contexts.items():
            if k == key:
                partition.append( c.getContext(self.xcns,self.useNuma) )
            else:
                if self.isI2Otarget(key,k):
                    partition.append( c.getContext(self.xcns,self.useNuma,False) )
        return partition


    def getConfigCmd(self,key):
        configns = 'urn:xdaq-soap:3.0'
        configCmd = ET.Element(QN(configns,'Configure'))
        configCmd.append( self.getPartition(key) )
        XMLtools.indent(configCmd)
        configString = ET.tostring(configCmd)
        configString = self.symbolMap.parse(configString)

        print("******** "+key[0]+":"+key[1]+" ********")
        print(configString)
        print("******** "+key[0]+":"+key[1]+" ********")

        return configString


class ConfigFromFile(Configuration):

    def __init__(self,symbolMap,configFile,fixPorts,useNuma,generateAtRU,dropAtRU,dropAtSocket,ferolMode):
        self.frlPorts = ['60500','60600']
        self.fedId2Port = {}
        Configuration.__init__(self,symbolMap,useNuma)

        if os.path.exists(configFile):
            ETroot = ET.parse(configFile).getroot()
        elif os.path.exists(configFile+".gz"):
            f = gzip.open(configFile+".gz") #gzip.open() doesn't support the context manager protocol needed for it to be used in a 'with' statement.
            ETroot = ET.parse(f).getroot()
            f.close()
        else:
            raise IOError(configFile+"(.gz) does not exist")

        self.xcns = re.match(r'\{(.*?)\}Partition',ETroot.tag).group(1) ## Extract xdaq namespace
        tid = 1

        for c in ETroot.getiterator(str(QN(self.xcns,'Context'))):
            try:
                url = c.attrib['url']
                role,count = self.urlToHostAndNumber(url)
                hostinfo = self.symbolMap.getHostInfo(role+str(count))
            except TypeError:
                #print("Found unknown role for "+url+", which will be ignored")
                continue

            if generateAtRU and role == "FEROLCONTROLLER":
                continue

            context = Context.Context(role,hostinfo)
            context.extractPolicy(c)

            maxbulksize="0x10000"
            portNb=0
            for endpoint in c.findall(QN(self.xcns,'Endpoint').text): ## all 'Endpoint's of this context
                if endpoint.attrib['protocol'] == "btcp":
                    maxbulksize = endpoint.attrib['maxbulksize']
                    if not fixPorts:
                        self.frlPorts[portNb] = endpoint.attrib['port']
                        portNb += 1

            for application in c.findall(QN(self.xcns,'Application').text): ## all 'Application's of this context
                if generateAtRU and application.attrib['class'] == "pt::blit::Application":
                    continue
                properties = self.getProperties(application)
                app = Application.Application(application.attrib['class'],application.attrib['instance'],properties)
                app.params['network'] = application.attrib['network']
                app.params['id'] = application.attrib['id']
                if app.params['class'] == 'pt::blit::Application':
                    app.params['frlHostname'] = context.hostinfo['frlHostname']
                    app.params['frlPort'] = self.frlPorts[0]
                    app.params['frlPort2'] = self.frlPorts[1]
                    app.params['maxbulksize'] = maxbulksize
                elif app.params['class'] == 'pt::ibv::Application':
                    app.params['protocol'] = 'ibv'
                    app.params['i2oHostname'] = context.hostinfo['i2oHostname']
                    app.params['i2oPort'] =  context.hostinfo['i2oPort']
                elif app.params['class'] == 'pt::utcp::Application':
                    app.params['protocol'] = 'atcp'
                    app.params['i2oHostname'] = context.hostinfo['i2oHostname']
                    app.params['i2oPort'] =  context.hostinfo['i2oPort']
                elif app.params['class'] == 'ferol::FerolController':
                    if fixPorts:
                        self.rewriteFerolProperties(app)
                    self.setOperationMode(app,ferolMode)
                elif app.params['class'].startswith('evb::'):
                    app.params['network'] = 'infini'
                    app.params['tid'] = str(tid)
                    tid += 1
                    if app.params['class'] == 'evb::EVM':
                        context.role = 'EVM'
                        self.evmFedId = self.getEvmFedId(app)
                        if count != '0':
                            raise Exception("The EVM must map to RU0, but maps to RU"+count)
                    if generateAtRU:
                        self.setLocalInput(app)
                    if dropAtRU:
                        self.dropInputData(app,'dropInputData')
                    if dropAtSocket:
                        self.dropInputData(app,'dropAtSocket')
                    elif fixPorts:
                        self.fixFerolPorts(app)
                context.applications.append(app)
            self.add(context)


    def getProperties(self,app):
        properties = []
        for child in app:
            if 'properties' in child.tag:
                for prop in child:
                    if 'rcmsStateListener' in prop.tag:
                        continue
                    param = self.getValues(prop)
                    if param[1] == 'Array':
                        val = []
                        for item in prop:
                            param[1] = self.getValues(item)[1]
                            if param[1] == 'Struct':
                                struct = []
                                for i in item:
                                    struct.append( tuple(self.getValues(i)) )
                                val.append(struct)
                            else:
                                val.append(item.text)
                        param[2] = val
                    properties.append( tuple(param) )
        return properties


    def getValues(self,element):
        xsins = "http://www.w3.org/2001/XMLSchema-instance"
        name = re.match(r'\{.*?\}(.*)',element.tag).group(1)
        type = re.match(r'.*?:(.*)',element.get(QN(xsins,'type'))).group(1)
        if element.text:
            value = element.text
        else:
            value = ''
        return [name,type,value]


    def rewriteFerolProperties(self,app):
        newProp = []
        for prop in app.properties:
            if prop[0] == 'enableStream0':
                enableStream0 = (prop[2] == 'true')
            elif prop[0] == 'enableStream1':
                enableStream1 = (prop[2] == 'true')
            elif prop[0] == 'expectedFedId_0':
                fedId0 = prop[2]
            elif prop[0] == 'expectedFedId_1':
                fedId1 = prop[2]
            elif prop[0] == 'slotNumber':
                slotNumber = int(prop[2])
            elif '_PORT_' not in prop[0] and 'lightStop' not in prop[0]:
                newProp.append(prop)
        newProp.append(('lightStop','boolean','true'))
        newProp.append(('DestinationIP','string',destHostInfo['frlHostname']))
        if enableStream0 and enableStream1 or slotNumber%2:
            newProp.append(('TCP_SOURCE_PORT_FED0','unsignedInt',self.frlPorts[0]))
            newProp.append(('TCP_SOURCE_PORT_FED1','unsignedInt',self.frlPorts[1]))
            newProp.append(('TCP_DESTINATION_PORT_FED0','unsignedInt',self.frlPorts[0]))
            newProp.append(('TCP_DESTINATION_PORT_FED1','unsignedInt',self.frlPorts[1]))
            self.fedId2Port[fedId0] = self.frlPorts[0]
            self.fedId2Port[fedId1] = self.frlPorts[1]
        else:
            newProp.append(('TCP_SOURCE_PORT_FED0','unsignedInt',self.frlPorts[1]))
            newProp.append(('TCP_SOURCE_PORT_FED1','unsignedInt',self.frlPorts[0]))
            newProp.append(('TCP_DESTINATION_PORT_FED0','unsignedInt',self.frlPorts[1]))
            newProp.append(('TCP_DESTINATION_PORT_FED1','unsignedInt',self.frlPorts[0]))
            self.fedId2Port[fedId0] = self.frlPorts[1]
            self.fedId2Port[fedId1] = self.frlPorts[0]
        app.properties = newProp


    def setOperationMode(self,app,ferolMode):
        newProp = []
        for prop in app.properties:
            if prop[0] == 'OperationMode':
                newProp.append(('OperationMode','string',ferolMode))
            else:
                newProp.append(prop)
        app.properties = newProp


    def dropInputData(self,app,dropMode='dropInputData'):
        if app.params['class'] in ('evb::EVM','evb::RU'):
            newProp = []
            for prop in app.properties:
                if not prop[0] == dropMode:
                    newProp.append(prop)
            newProp.append((dropMode,'boolean','true'))
            app.properties = newProp


    def setLocalInput(self,app):
        if app.params['class'] in ('evb::EVM','evb::RU'):
            newProp = [('computeCRC','boolean','true')]
            for prop in app.properties:
                if prop[0] == 'inputSource':
                    newProp.append(('inputSource','string','Local'))
                elif prop[0] not in ('ferolSources','computeCRC'):
                    newProp.append(prop)
            app.properties = newProp


    def fixFerolPorts(self,app):
        newProp = []
        for prop in app.properties:
            if prop[0] == 'ferolSources':
                sources = []
                for source in prop[2]:
                    items = []
                    for item in source:
                        if item[0] == 'fedId':
                            fedId = item[2]
                        if item[0] != 'port':
                            items.append(item)
                    items.append(('port','unsignedInt',self.fedId2Port[fedId]))
                    sources.append(items)
                newProp.append(('ferolSources','Struct',sources))
            else:
                newProp.append(prop)
        app.properties = newProp


    def urlToHostAndNumber(self,url):
        """
        Converts context url strings like
        'http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT'
        to a pair of strings of hosttype and index. I.e. 'RU' and '0' in this case.
        """
        pattern = re.compile(r'http://([A-Z0-9]*?)([0-9]+)_SOAP_HOST_NAME:.*')
        match = pattern.match(url)
        if match:
            return match.group(1), match.group(2) ## so h will be RU/BU/EVM/FEROLCONTROLLER/..., n will be 0,1,2,3,...
        return None


def getApplicationStates(configuration):
    # @param configuration must be an instance of Configuration.
    # 
    # @return a dict of (application type) -> (list of application states)
    # where application type is e.g. 'RU' etc. and application state
    # is e.g. 'Failed' etc.
    # 
    # this is e.g. used to check if any of the applications went into
    # 'Failed' state

    result = {}

    import messengers

    for appType, appList in configuration.applications.items():
        result[appType] = []

        for application in appList:

            try:
                state = messengers.getStateName(**application)
            except Exception, ex:
                # e.g. connection refused
                state = None

            result[appType].append(state)

    return result




if __name__ == "__main__":
    symbolMap = SymbolMap.SymbolMap(os.environ["EVB_TESTER_HOME"]+"/cdaq/20170131/canon_1str_4x4/symbolMap.txt")
    config = ConfigFromFile(symbolMap,os.environ["EVB_TESTER_HOME"]+"/cdaq/20170131/canon_1str_4x4/canon_1str_4x4.xml",
                                fixPorts=False,useNuma=True,generateAtRU=False,dropAtRU=False,dropAtSocket=False,ferolMode=True)
    #print(config.contexts)
    for key in config.contexts.keys():
        config.getConfigCmd(key)
    print(config.applications)
    print(config.ptUtcp)
