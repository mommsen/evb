import copy
import os
import re
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import Context

class Configuration():

    def __init__(self):
        self.contexts = {}
        self.configCmd = ""
        self.ptUtcp = []
        self.ptFrl = []
        self.applications = {}


    def add(self,context):
        contextKey = (context.apps['soapHostname'],context.apps['soapPort'],context.apps['launcherPort'])
        self.contexts[contextKey] = context
        appInfo = dict((key,context.apps[key]) for key in ('app','instance','soapHostname','soapPort'))
        if context.role not in self.applications:
            self.applications[context.role] = []
        self.applications[context.role].append(copy.deepcopy(appInfo))
        for nested in context.nestedContexts:
            if nested.role not in self.applications:
                self.applications[nested.role] = []
            appInfo['app'] = nested.apps['app']
            appInfo['instance'] = nested.apps['instance']
            self.applications[nested.role].append(copy.deepcopy(appInfo))
        if context.apps['ptUtcp'] is not None:
            appInfo['app'] = context.apps['ptUtcp']
            appInfo['instance'] = context.apps['ptInstance']
            self.ptUtcp.append(copy.deepcopy(appInfo))
        if context.apps['ptFrl'] is not None:
            appInfo['app'] = context.apps['ptFrl']
            appInfo['instance'] = context.apps['ptInstance']
            self.ptFrl.append(appInfo)


    def getPartition(self):
        print self.contexts
        print self.applications
        partition = """
  <xc:Partition xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xc="http://xdaq.web.cern.ch/xdaq/xsd/2004/XMLConfiguration-30" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">

    <i2o:protocol xmlns:i2o="http://xdaq.web.cern.ch/xdaq/xsd/2004/I2OConfiguration-30">
"""
        for context in self.contexts.values():
            partition += context.getTarget()
            for nested in context.nestedContexts:
                partition += nested.getTarget()

        partition += "    </i2o:protocol>\n\n"

        for context in self.contexts.values():
            partition += """    <xc:Context url="http://%(soapHostname)s:%(soapPort)s">""" % context.apps
            partition += context.getConfig()
            for nested in context.nestedContexts:
                partition += nested.getConfigForApplication()
            partition += "    </xc:Context>\n\n"

        partition += "  </xc:Partition>\n"

        return partition

    def getConfigCmd(self,key):
        if len(self.configCmd) == 0:
            self.configCmd = """<xdaq:Configure xmlns:xdaq=\"urn:xdaq-soap:3.0\">"""
            self.configCmd += self.getPartition()
            self.configCmd += "</xdaq:Configure>"

            print("**************************************")
            print(self.configCmd)
            print("**************************************")

        return self.configCmd
