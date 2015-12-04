import httplib
import socket
import urllib
from xml.dom import minidom


soapTemplate="""
<soap-env:Envelope
soap-env:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xmlns:xsd="http://www.w3.org/2001/XMLSchema"
xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/">
<soap-env:Header/>
<soap-env:Body>%s</soap-env:Body>
</soap-env:Envelope>
"""

class SOAPexception(Exception):
    pass


def sendCmdToLauncher(cmd,soapHostname,launcherPort,soapPort=None):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((soapHostname,int(launcherPort)))
        if soapPort is None:
            s.send(cmd)
        else:
            s.send(cmd+':'+soapPort)
        reply = s.recv(1024)
        s.close()
        return reply
    except socket.error:
        pass


def webPing(soapHostname,soapPort):
    code = None
    try:
        code = urllib.urlopen("http://"+soapHostname+":"+soapPort+"/urn:xdaq-application:lid=3").getcode()
    except IOError:
        pass
    return (code == 200)


def sendSoapMessage(soapHostname,soapPort,urn,body):
    soapMessage = soapTemplate%(body)
    headers = {"Content-Type":"text/xml",
               "Content-Description":"SOAP Message",
               "SOAPAction":urn}
    server = httplib.HTTPConnection(soapHostname,soapPort)
    #server.set_debuglevel(4)
    server.request("POST","",soapMessage,headers)
    response = server.getresponse().read()
    return minidom.parseString(response)


def sendCmdToApp(command,soapHostname,soapPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    response = sendSoapMessage(soapHostname,soapPort,urn,"<xdaq:"+command+" xmlns:xdaq=\"urn:xdaq-soap:3.0\"/>")
    xdaqResponse = response.getElementsByTagName('xdaq:'+command+'Response')
    if len(xdaqResponse) == 0:
        xdaqResponse = response.getElementsByTagName('xdaq:'+command.lower()+'Response')
    if len(xdaqResponse) != 1:
        raise(SOAPexception("Did not get a proper FSM response from "+app+str(instance)+":\n"+response.toprettyxml()))
    try:
        return xdaqResponse[0].firstChild.attributes['xdaq:stateName'].value
    except (AttributeError,KeyError):
        return "unknown"


def setParam(paramName,paramType,paramValue,soapHostname,soapPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    paramSet = """<xdaq:ParameterSet xmlns:xdaq=\"urn:xdaq-soap:3.0\"><p:properties xmlns:p="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct"><p:%(paramName)s xsi:type="xsd:%(paramType)s">%(paramValue)s</p:%(paramName)s></p:properties></xdaq:ParameterSet>""" % {'paramName':paramName,'paramType':paramType,'paramValue':str(paramValue),'app':app}
    response = sendSoapMessage(soapHostname,soapPort,urn,paramSet)
    xdaqResponse = response.getElementsByTagName('xdaq:ParameterSetResponse')
    if len(xdaqResponse) != 1:
        raise(SOAPexception("Failed to set "+paramName+" ("+paramType+") to "+str(paramValue)+" on "+app+":"+instance+":\n"
                            +response.toprettyxml()))


def getParam(paramName,paramType,soapHostname,soapPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    paramGet = """<xdaq:ParameterGet xmlns:xdaq=\"urn:xdaq-soap:3.0\"><p:properties xmlns:p="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct"><p:%(paramName)s xsi:type="xsd:%(paramType)s"/></p:properties></xdaq:ParameterGet>""" % {'paramName':paramName,'paramType':paramType,'app':app}
    response = sendSoapMessage(soapHostname,soapPort,urn,paramGet)
    xdaqResponse = response.getElementsByTagName('p:'+paramName)
    if len(xdaqResponse) != 1:
        raise(SOAPexception("ParameterGetResponse response from "+app+str(instance)+" is missing "+paramName+" ("+paramType+"):\n"
                            +response.toprettyxml()))
    value = xdaqResponse[0].firstChild.data
    try:
        return int(value)
    except ValueError:
        return value


def getStateName(soapHostname,soapPort,app,instance):
    return getParam("stateName","string",soapHostname,soapPort,app,instance)
