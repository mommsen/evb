#!/usr/bin/env python

import collections
import datetime
import os
import re
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import QName as QN

import XMLtools


def getConfig(args):
    # Call configurator with options. The output shall be placed in the temporary configFile
    configFile = "/Users/mommsen/cmsusr/daq2Test/daq2val/x86_64_slc6/daq2val/29083638/configuration.xml"
    ETroot = ET.parse(configFile).getroot()
    #os.remove(configFile)
    return ETroot


def getHostFromURL(url):
    pattern = re.compile(r'http://(.*):.*')
    return pattern.match(url).group(1)


def getSymbolMap(config):
    symbolMap = collections.OrderedDict()
    nFRL = 0
    nRU = 1
    nBU = 0
    xcns = re.match(r'\{(.*?)\}Partition',config.tag).group(1) ## Extract xdaq namespace
    for c in config.getiterator(str(QN(xcns,'Context'))):
        for a in c.getiterator(str(QN(xcns,'Application'))):
            if a.attrib['class'].startswith(('evb::test::DummyFEROL','ferol::')):
                hostType = 'FEROLCONTROLLER'+str(nFRL)
                nFRL += 1
            elif a.attrib['class'] == 'evb::EVM':
                hostType = 'RU0'
            elif a.attrib['class'] == 'evb::RU':
                hostType = 'RU'+str(nRU)
                nRU += 1
            elif a.attrib['class'] == 'evb::BU':
                hostType = 'BU'+str(nBU)
                nBU += 1
        symbolMap[hostType+'_SOAP_HOST_NAME'] = getHostFromURL(c.attrib['url'])
        c.attrib['url'] = 'http://'+hostType+'_SOAP_HOST_NAME'+':'+hostType+'_SOAP_PORT'
        for e in c.getiterator(str(QN(xcns,'Endpoint'))):
            if e.attrib['service'] == 'i2o':
                name = hostType+'_I2O'
                e.attrib['port'] = name+'_PORT'
            elif e.attrib['service'] == 'blit':
                name = hostType+'_FRL'
            symbolMap[name+'_HOST_NAME'] = e.attrib['hostname']
            e.attrib['hostname'] = name+'_HOST_NAME'
    return symbolMap


def createDir(outputDir):
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


def writeSymbolMap(symbolMap,outputDir):
    with open(outputDir+'/symbolMap.txt','w') as symbolMapFile:
        symbolMapFile.write("LAUNCHER_BASE_PORT 17777\n")
        symbolMapFile.write("SOAP_BASE_PORT 25000\n")
        symbolMapFile.write("I2O_BASE_PORT 54320\n")
        symbolMapFile.write("FRL_BASE_PORT 55320\n")
        symbolMapFile.write("\n")
        for key in symbolMap:
            symbolMapFile.write("%s %s\n" % (key,symbolMap[key]))


def writeConfig(config,outputDir):
    configName = os.path.split(outputDir)[1]
    XMLtools.indent(config)
    with open(outputDir+'/'+configName+'.xml','w') as configFile:
        configFile.write( ET.tostring(config) )


if __name__ == "__main__":
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("eqset",help="Equipement set to use")
    parser.add_argument("hostnames",help="Filename with RU/BU hosts to use")
    parser.add_argument("output",help="Path to output directory")
    args = vars( parser.parse_args() )
    config = getConfig(args)
    symbolMap = getSymbolMap(config)
    createDir(args['output'])
    writeSymbolMap(symbolMap,args['output'])
    writeConfig(config,args['output'])
