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
        self.applications = {}
        self.xcns = 'http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30'


    def add(self,context):
        appInfo = dict((key,context.hostinfo[key]) for key in ('soapHostname','soapPort'))
        contextKey = (appInfo['soapHostname'],appInfo['soapPort'])
        self.contexts[contextKey] = context
        for app in context.applications:
            appInfo['app'] = app.params['class']
            appInfo['instance'] = app.params['instance']
            if app.params['class'].startswith(('evb::test::DummyFEROL','ferol::')):
                self.addAppInfoToApplications('FEROL',appInfo)
            elif app.params['class'] == 'evb::EVM':
                self.addAppInfoToApplications('EVM',appInfo)
            elif app.params['class'] == 'evb::RU':
                self.addAppInfoToApplications('RU',appInfo)
            elif app.params['class'] == 'evb::BU':
                self.addAppInfoToApplications('BU',appInfo)
            elif app.params['class'].startswith( ('pt::utcp','pt::ibv') ):
                self.ptUtcp.append( copy.deepcopy(appInfo) )


    def addAppInfoToApplications(self,role,appInfo):
        if role not in self.applications:
            self.applications[role] = []
        self.applications[role].append( copy.deepcopy(appInfo) )


    def isI2Otarget(self,myKey,otherKey):
        if myKey == otherKey:
            return True
        if self.contexts[myKey].role == 'EVM' and self.contexts[otherKey].role in ('RU','BU'):
            return True
        if self.contexts[myKey].role == 'RU' and self.contexts[otherKey].role == 'BU':
            return True
        if self.contexts[myKey].role == 'BU' and self.contexts[otherKey].role == 'EVM':
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
                partition.append( c.getContext(self.xcns) )
            else:
                if self.isI2Otarget(key,k):
                    partition.append( c.getContext(self.xcns,False) )
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
        ETroot = ET.parse(configFile).getroot()

        self.xcns = re.match(r'\{(.*?)\}Partition',ETroot.tag).group(1) ## Extract xdaq namespace
        tid = 0

        for c in ETroot.getiterator(str(QN(self.xcns,'Context'))):
            context = Context.Context()
            #for child in c.getchildren():
            #    print ET.tostring(child)
            context.role,count = self.urlToHostAndNumber(c.attrib['url'])
            context.apps = self.symbolMap.getHostInfo(context.role+str(count))

            for application in c.findall(QN(self.xcns,'Application').text): ## all 'Application's of this context
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
    os.environ["EVB_SYMBOL_MAP"] = 'daq2valSymbolMap.txt'
    symbolMap = SymbolMap.SymbolMap(os.environ["EVB_TESTER_HOME"]+"/scans/")
    config = ConfigFromFile(symbolMap,os.environ["EVB_TESTER_HOME"]+"/scans/8s8fx1x2_evb_ibv_COL.xml")
    print(config.contexts)
    print(config.applications)
    print(config.ptUtcp)
    for key in config.contexts.keys():
        config.getConfigCmd(key)
