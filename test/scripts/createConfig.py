#!/usr/bin/env python

import collections
import datetime
import os
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import XMLtools


class CreateConfig:

    def __init__(self,args):
        self.args = vars(args)
        self.infoStr = "Command used:\n%s\n\n" % " ".join(sys.argv)


    def doIt(self):
        config = self.getConfig()
        symbolMap = self.getSymbolMap(config)
        self.createDir(self.args['output'])
        self.writeSymbolMap(symbolMap,self.args['output'])
        self.writeConfig(config,self.args['output'])
        self.writeInfo(self.args['output'])


    def getConfig(self):
        # Call configurator with options. The output shall be placed in the temporary configFile
        #configFile = "/Users/mommsen/cmsusr/daq2Test/daq2val/x86_64_slc6/daq2val/29083638/configuration.xml"
        configFile = "/tmp/"+time.strftime('%d%H%M%S')+".xml"
        javaCmd = ['java','-Doracle.net.tns_admin=/etc',
                   '-jar','configurator_new_workflow_test_v2.jar',
                   '--properties',os.environ['HOME']+'/CONFIGURATOR.properties',
                   '--batch',
                   '--account','daqlocal','--site','daq2',
                   '--buildDPSet',
                   '--fbSet',self.args['fbset'],
                   '--mainRCMSHost','cmsrc-daq.cms',
                   '--routingOptimizer','GREEDY_OPTIMIZER',
                   '--nBU',self.args['nBU'],
                   '--makeConfig',
                   '--swt',self.args['swt'],
                   '--saveXML',configFile]
        configCmd = " ".join(javaCmd)
        self.infoStr += "Java command used:\n%s\n\n" % configCmd
        print("Running "+configCmd+":")
        p = subprocess.Popen(javaCmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        while True:
            out = p.stdout.read(1)
            if out == '' and p.poll() != None:
                break
            sys.stdout.write(out)
            sys.stdout.flush()
        try:
            ETroot = ET.parse(configFile).getroot()
            os.remove(configFile)
        except IOError:
            sys.exit(1)
        return ETroot


    def getHostFromURL(self,url):
        pattern = re.compile(r'http://(.*):.*')
        return pattern.match(url).group(1)


    def getFedIds(self,app):
        fedIds = []
        for child in app:
            if 'properties' in child.tag:
                for prop in child:
                    if 'fedSourceIds' in prop.tag:
                        for item in prop:
                            fedIds.append(item.text)
        return fedIds


    def getSymbolMap(self,config):
        self.infoStr += "Hosts:\n"
        symbolMap = collections.OrderedDict()
        nFRL = 0
        nRU = 1
        nBU = 0
        xcns = re.match(r'\{(.*?)\}Partition',config.tag).group(1) ## Extract xdaq namespace
        for context in config.getiterator(str(QN(xcns,'Context'))):
            for app in context.getiterator(str(QN(xcns,'Application'))):
                hostType = None
                if app.attrib['class'].startswith(('evb::test::DummyFEROL','ferol::')):
                    hostType = 'FEROLCONTROLLER'+str(nFRL)
                    nFRL += 1
                elif app.attrib['class'] == 'evb::EVM':
                    hostType = 'RU0'
                    fedIds = self.getFedIds(app)
                elif app.attrib['class'] == 'evb::RU':
                    hostType = 'RU'+str(nRU)
                    fedIds = self.getFedIds(app)
                    nRU += 1
                elif app.attrib['class'] == 'evb::BU':
                    hostType = 'BU'+str(nBU)
                    nBU += 1
            if hostType:
                hostName = self.getHostFromURL(context.attrib['url'])
                symbolMap[hostType+'_SOAP_HOST_NAME'] = hostName
                self.infoStr += "   "+hostName
                if 'RU' in hostType:
                    self.infoStr += ' with %d FEDs: %s' % (len(fedIds),",".join(fedIds))
                    if hostType == 'RU0':
                        self.infoStr += ' (EVM)'
                self.infoStr += '\n'
                context.attrib['url'] = 'http://'+hostType+'_SOAP_HOST_NAME'+':'+hostType+'_SOAP_PORT'
                for endpoint in context.getiterator(str(QN(xcns,'Endpoint'))):
                    if endpoint.attrib['service'] == 'i2o':
                        name = hostType+'_I2O'
                        endpoint.attrib['port'] = name+'_PORT'
                    elif endpoint.attrib['service'] == 'blit':
                        name = hostType+'_FRL'
                    symbolMap[name+'_HOST_NAME'] = endpoint.attrib['hostname']
                    endpoint.attrib['hostname'] = name+'_HOST_NAME'
        return symbolMap


    def createDir(self,outputDir):
        try:
            os.makedirs(outputDir)
        except OSError:
            t = os.path.getmtime(outputDir)
            str = datetime.datetime.fromtimestamp(t).strftime("%Y%m%d_%H%M%S")
            bakDir = outputDir+'/'+str
            os.makedirs(bakDir)
            for file in os.listdir(outputDir):
                if os.path.isfile(outputDir+'/'+file):
                    os.rename(outputDir+'/'+file,bakDir+'/'+file)


    def writeSymbolMap(self,symbolMap,outputDir):
        with open(outputDir+'/symbolMap.txt','w') as symbolMapFile:
            symbolMapFile.write("LAUNCHER_BASE_PORT 17777\n")
            symbolMapFile.write("SOAP_BASE_PORT 25000\n")
            symbolMapFile.write("I2O_BASE_PORT 54320\n")
            symbolMapFile.write("FRL_BASE_PORT 55320\n")
            symbolMapFile.write("\n")
            for key in symbolMap:
                symbolMapFile.write("%s %s\n" % (key,symbolMap[key]))


    def writeConfig(self,config,outputDir):
        configName = os.path.split(outputDir)[1]
        XMLtools.indent(config)
        with open(outputDir+'/'+configName+'.xml','w') as configFile:
            configFile.write( ET.tostring(config) )


    def writeInfo(self,outputDir):
        print("\n\nSummary:\n")
        print(self.infoStr)
        with open(outputDir+'/setup.info','w') as infoFile:
            infoFile.write(self.infoStr)


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("fbset",help="Fed-builder set to use")
    parser.add_argument("swt",help="Software template to use")
    parser.add_argument("nBU",help="Number of BUs")
    parser.add_argument("output",help="Path to output directory")
    createConfig = CreateConfig( parser.parse_args() )
    createConfig.doIt()
