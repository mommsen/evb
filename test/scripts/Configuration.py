import copy
import os
import re
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Context
import SymbolMap

class Configuration():

    def __init__(self,symbolMap):
        self.symbolMap = symbolMap
        self.contexts = {}
        self.ptUtcp = []
        self.ptFrl = []
        self.applications = {}


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


    def getTargets(self,key):
        myRole = self.contexts[key].role
        target = ""
        for k,c in self.contexts.items():
            if self.isI2Otarget(key,k):
                try:
                    target += """      <i2o:target class="%(app)s" instance="%(instance)s" tid="%(tid)s"/>\n""" % c.apps
                except KeyError: #No I2O tid assigned
                    pass
        if len(target) == 0:
            return ""
        else:
            return """    <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">\n""" + target +\
                     "    </i2o:protocol>\n\n"


    def getPartition(self,key):
        partition = """  <xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">\n"""

        partition += self.getTargets(key)

        for k,c in self.contexts.items():
            context = ""
            if k == key:
                context = c.getConfig()
                for nested in c.nestedContexts:
                    context += nested.getConfigForApplication()
            else:
                if self.isI2Otarget(key,k):
                    try:
                        context = """      <xc:Endpoint protocol="%(ptProtocol)s" service="i2o" hostname="%(i2oHostname)s" port="%(i2oPort)s" network="%(network)s"/>
      <xc:Application class="%(app)s" id="%(appId)s" instance="%(instance)s" network="%(network)s"/>
""" % c.apps
                    except KeyError: #No I2O tid assigned
                        pass
            if len(context) > 0:
                partition += """    <xc:Context url="http://%(soapHostname)s:%(soapPort)s">\n""" % c.apps
                partition += context
                partition += "    </xc:Context>\n\n"

        partition += "  </xc:Partition>\n"

        return partition


    def getConfigCmd(self,key):
        configCmd = """<xdaq:Configure xmlns:xdaq=\"urn:xdaq-soap:3.0\">\n"""
        configCmd += self.getPartition(key)
        configCmd += "</xdaq:Configure>"

        print("******** "+key[0]+":"+key[1]+" ********")
        print(configCmd)
        print("******** "+key[0]+":"+key[1]+" ********")

        return configCmd


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

            for application in c.findall(QN(self.xcns, 'Application').text): ## all 'Application's of this context
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
