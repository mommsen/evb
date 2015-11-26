import copy
import os
import re
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Context
import SymbolMap
import XMLtools

class Configuration():

    def __init__(self,symbolMap):
        self.symbolMap = symbolMap
        self.contexts = {}
        self.ptUtcp = []
        self.ptFrl = []
        self.applications = {}
        self.xcns = 'http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30'


    def add(self,context):
        appInfo = dict((key,context.apps[key]) for key in ('app','instance','soapHostname','soapPort'))
        contextKey = (appInfo['soapHostname'],appInfo['soapPort'])
        self.contexts[contextKey] = context
        if context.role not in self.applications:
            self.applications[context.role] = []
        self.applications[context.role].append(copy.deepcopy(appInfo))
        for nested in context.nestedContexts:
            if nested.role not in self.applications:
                self.applications[nested.role] = []
            appInfo['app'] = nested.apps['app']
            appInfo['instance'] = nested.apps['instance']
            self.applications[nested.role].append(copy.deepcopy(appInfo))
        try:
            if context.apps['ptProtocol'] == 'atcp':
                appInfo['app'] = 'pt::utcp::Application'
            elif context.apps['ptProtocol'] == 'ibv':
                appInfo['app'] = 'pt::ibv::Application'
            appInfo['instance'] = context.apps['ptInstance']
            self.ptUtcp.append(copy.deepcopy(appInfo))
        except KeyError:
            pass
        try:
            appInfo['app'] = context.apps['ptFrl']
            appInfo['instance'] = context.apps['ptInstance']
            self.ptFrl.append(appInfo)
        except KeyError:
            pass


    def isI2Otarget(self,myKey,otherKey):
        if myKey == otherKey:
            return True
        if self.contexts[myKey].role == 'EVM':
            return True
        if self.contexts[myKey].role == 'RU' and self.contexts[otherKey].role == 'BU':
            return True
        if self.contexts[myKey].role == 'BU' and self.contexts[otherKey].role == 'EVM':
            return True
        return False


    def getTargetElement(self,ns,apps):
        target = ET.Element(QN(ns,'target'))
        target.set('class',apps['app'])
        target.set('instance',str(apps['instance']))
        target.set('tid',str(apps['tid']))
        return target


    def getTargets(self,key):
        myRole = self.contexts[key].role
        i2ons = "http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30"
        protocol = ET.Element(QN(i2ons,'protocol'))
        for k,c in self.contexts.items():
            if self.isI2Otarget(key,k):
                try:
                    protocol.append( self.getTargetElement(i2ons,c.apps) )
                except KeyError: #No I2O tid assigned
                    pass
                for nested in c.nestedContexts:
                    protocol.append( self.getTargetElement(i2ons,nested.apps) )
        return protocol


    def getPartition(self,key):

        partition = ET.Element(QN(self.xcns,'Partition'))

        protocol = self.getTargets(key)
        if len(protocol) > 0:
            partition.append(protocol)

        for k,c in self.contexts.items():
            context = ET.Element(QN(self.xcns,'Context'))
            context.set('url',"http://%(soapHostname)s:%(soapPort)s" % c.apps)
            if k == key:
                c.addInContext(context,self.xcns)
                for nested in c.nestedContexts:
                    nested.addConfigForApplication(context,self.xcns)
            else:
                if self.isI2Otarget(key,k):
                    try:
                        context.append( c.getEndpointContext(self.xcns) )
                        context.append( c.getApplicationContext(self.xcns) )
                    except KeyError: #No I2O tid assigned
                        pass
            if len(context) > 0:
                partition.append(context)

        return partition


    def getConfigCmd(self,key):
        configns = 'urn:xdaq-soap:3.0'
        configCmd = ET.Element(QN(configns,'Configure'))
        configCmd.append( self.getPartition(key) )
        XMLtools.indent(configCmd)
        configString = ET.tostring(configCmd)

        print("******** "+key[0]+":"+key[1]+" ********")
        print(configString)
        print("******** "+key[0]+":"+key[1]+" ********")

        return configString


class ConfigFromFile(Configuration):

    def __init__(self,symbolMap,configFile):
        Configuration.__init__(self,symbolMap)
        self.config   = ET.parse(configFile)
        self.ETroot   = self.config.getroot()

        self.xcns     = re.match(r'\{(.*?)\}Partition', self.ETroot.tag).group(1) ## Extract xdaq namespace
        tid = 0

        for c in self.ETroot.getiterator(str(QN(self.xcns,'Context'))):
            context = Context.Context()
            for child in c.getchildren():
                context.config += ET.tostring(child)
            context.role,count = self.urlToHostAndNumber(c.attrib['url'])
            context.apps = self.symbolMap.getHostInfo(context.role+str(count))

            for application in c.findall(QN(self.xcns, 'Application')): ## all 'Application's of this context
                app = application.attrib['class']
                if app.startswith(('evb::','ferol::')):
                    if app == 'evb::EVM':
                        context.role = 'EVM'
                    context.apps['app'] = app
                    context.apps['instance'] = application.attrib['instance']
                    context.apps['appId'] = application.attrib['id']
                elif app == 'pt::ibv::Application':
                    context.apps['ptProtocol'] = 'ibv'
                    context.apps['network'] = 'infini'
                    context.apps['ptInstance'] = application.attrib['instance']
                    context.apps['tid'] = tid
                    tid += 1
                elif app == 'pt::utcp::Application':
                    context.apps['ptProtocol'] = 'atcp'
                    context.apps['network'] = 'tcp'
                    context.apps['ptInstance'] = application.attrib['instance']
                    context.apps['tid'] = tid
                    tid += 1
                elif app == 'pt::frl::Application':
                    context.apps['ptFrl'] = app
                    context.apps['ptInstance'] = application.attrib['instance']
            self.add(context)


    def urlToHostAndNumber(self,url):
        """
        Converts context url strings like
        'http://RU0_SOAP_HOST_NAME:RU0_SOAP_PORT'
        to a pair of strings of hosttype and index. I.e. 'RU' and '0' in this case.
        """
        pattern = re.compile(r'http://([A-Z_0-9]*?)([0-9]+)_SOAP_HOST_NAME:.*')
        h,n = pattern.match(url).group(1), pattern.match(url).group(2) ## so h will be RU/BU/EVM/FEROLCONTROLLER/..., n will be 0,1,2,3,...
        return h,n


if __name__ == "__main__":
    symbolMap = SymbolMap.SymbolMap(os.environ["EVB_TESTER_HOME"]+"/scans/")
    config = ConfigFromFile(symbolMap,os.environ["EVB_TESTER_HOME"]+"/scans/8s8fx1x2_evb_ibv_COL.xml")
    print(config.contexts)
    print(config.applications)
    print(config.ptUtcp)
    for key in config.contexts.keys():
        config.getConfigCmd(key)
