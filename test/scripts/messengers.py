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
xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/">
<soap-env:Header/>
<soap-env:Body>%s</soap-env:Body>
</soap-env:Envelope>
"""

class SOAPexception(Exception):
    pass


def sendCmdToLauncher(cmd,soapHostname,launcherPort,soapPort=None,testname=""):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((soapHostname,int(launcherPort)))
        if soapPort is None:
            s.send(cmd+" "+testname)
        else:
            s.send(cmd+':'+soapPort+" "+testname)
        response = []
        while True:
            reply = s.recv(1024)
            if not reply: break
            response.append(reply)
        s.close()
        return ''.join(response)
    except socket.error as e:
        return "Failed to contact launcher: "+e.strerror


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
    try:
        response = server.getresponse().read()
        return minidom.parseString(response)
    except (httplib.HTTPException,socket.error) as e:
        raise SOAPexception("Received bad response from "+soapHostname+":"+soapPort+": "+str(e))


def sendCmdToApp(command,soapHostname,soapPort,launcherPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    response = sendSoapMessage(soapHostname,soapPort,urn,"<xdaq:"+command+" xmlns:xdaq=\"urn:xdaq-soap:3.0\"/>")
    try:
        xdaqResponse = response.getElementsByTagName('xdaq:'+command+'Response')
    except AttributeError:
        raise(SOAPexception("Did not get a proper FSM response from "+app+str(instance)+":\n"+response.toprettyxml()))
    if len(xdaqResponse) == 0:
        xdaqResponse = response.getElementsByTagName('xdaq:'+command.lower()+'Response')
    if len(xdaqResponse) != 1:
        raise(SOAPexception("Did not get a proper FSM response from "+app+str(instance)+":\n"+response.toprettyxml()))
    try:
        return xdaqResponse[0].firstChild.attributes['xdaq:stateName'].value
    except (AttributeError,KeyError):
        return "unknown"


def setParam(paramName,paramType,paramValue,soapHostname,soapPort,launcherPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    if isinstance(paramValue,(tuple,list)):
        param = """<p:%(paramName)s xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[%(len)d]">""" % {'paramName':paramName,'len':len(paramValue)}
        for pos,val in enumerate(paramValue):
            if isinstance(val,(tuple,list)):
                param += """<p:item soapenc:position="[%(pos)d]" xsi:type="soapenc:Struct">""" % {'pos':pos}
                for item in val:
                    param += """<p:%(paramName)s xsi:type="xsd:%(paramType)s">%(paramValue)s</p:%(paramName)s>""" % {'paramName':item[0],'paramType':item[1],'paramValue':str(item[2])}
                param += """</p:item>"""
            else:
                param += """<p:item soapenc:position="[%(pos)d]" xsi:type="xsd:%(paramType)s">%(paramValue)s</p:item>""" % {'pos':pos,'paramType':paramType,'paramValue':str(val)}
        param += """</p:%(paramName)s>""" % {'paramName':paramName}
    else:
        param = """<p:%(paramName)s xsi:type="xsd:%(paramType)s">%(paramValue)s</p:%(paramName)s>""" % {'paramName':paramName,'paramType':paramType,'paramValue':str(paramValue)}
    paramSet = """<xdaq:ParameterSet xmlns:xdaq=\"urn:xdaq-soap:3.0\"><p:properties xmlns:p="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">%(param)s</p:properties></xdaq:ParameterSet>""" % {'app':app,'param':param}
    response = sendSoapMessage(soapHostname,soapPort,urn,paramSet)
    xdaqResponse = response.getElementsByTagName('xdaq:ParameterSetResponse')
    if len(xdaqResponse) != 1:
        raise(SOAPexception("Failed to set "+paramName+" ("+paramType+") to "+str(paramValue)+" on "+app+":"+instance+":\n"
                            +response.toprettyxml()))


def getParam(paramName,paramType,soapHostname,soapPort,launcherPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    if paramType == "Array":
        param = """<p:%(paramName)s xsi:type="soapenc:Array" soapenc:arrayType="xsd:ur-type[]"/>""" % {'paramName':paramName}
    else:
        param = """<p:%(paramName)s xsi:type="xsd:%(paramType)s"/>""" % {'paramName':paramName,'paramType':paramType}
    paramGet = """<xdaq:ParameterGet xmlns:xdaq=\"urn:xdaq-soap:3.0\"><p:properties xmlns:p="urn:xdaq-application:%(app)s" xsi:type="soapenc:Struct">%(param)s</p:properties></xdaq:ParameterGet>""" % {'app':app,'param':param}
    response = sendSoapMessage(soapHostname,soapPort,urn,paramGet)
    #print response.toprettyxml()
    xdaqResponse = response.getElementsByTagName('p:'+paramName)
    if len(xdaqResponse) != 1:
        raise(SOAPexception("ParameterGetResponse response from "+app+str(instance)+" is missing "+paramName+" ("+paramType+"):\n"
                            +response.toprettyxml()))
    if not xdaqResponse[0].hasChildNodes():
        if paramType == "Array":
            return []
        else:
            return None
    try:
        value = xdaqResponse[0].firstChild.data
        try:
            return int(value)
        except ValueError:
            return value
    except AttributeError:
        values = []
        for node in xdaqResponse[0].childNodes:
            try:
                value = node.firstChild.data
                try:
                    values.append(int(value))
                except ValueError:
                    values.append(value)
            except AttributeError:
                val = []
                for item in node.childNodes:
                    type = item.attributes['xsi:type'].value[4:]
                    value = item.firstChild.data
                    try:
                        val.append((item.localName,type,int(value)))
                    except ValueError:
                        val.append((item.localName,type,value))
                values.append(val)
        return values


def readItem(device,offset,item,soapHostname,soapPort,launcherPort,app,instance):
    urn = "urn:xdaq-application:class="+app+",instance="+str(instance)
    readItem = """<xdaq:ReadItem xmlns:xdaq="urn:xdaq-soap:3.0" device="%(device)s" offset="%(offset)s" item="%(item)s"/>""" % {'device':device,'offset':offset,'item':item}
    response = sendSoapMessage(soapHostname,soapPort,urn,readItem)
    xdaqResponse = response.getElementsByTagName('xdaq:ReadItemResponse')
    if len(xdaqResponse) != 1:
        raise(SOAPexception("Failed to read item "+item+" from "+app+str(instance)+":\n"
                            +response.toprettyxml()))
    value = xdaqResponse[0].firstChild.data
    try:
        return int(value)
    except ValueError:
        return value


def getStateName(soapHostname,soapPort,launcherPort,app,instance):
    return getParam("stateName","string",soapHostname,soapPort,launcherPort,app,instance)


if __name__ == "__main__":
    print getParam('eventCount','unsignedLong','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')

    playbackDataFile = getParam('playbackDataFile','string','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    if playbackDataFile:
        playbackDataFile += "_test"
    else:
        playbackDataFile = "test"
    setParam('playbackDataFile','string',playbackDataFile,'kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    if playbackDataFile != getParam('playbackDataFile','string','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0'):
        print("String setting failed!")

    fedIds = getParam('fedSourceIds','Array','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    newIds = []
    for id in fedIds:
        newIds.append(id+10)
    setParam('fedSourceIds','unsignedInt',newIds,'kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    if newIds != getParam('fedSourceIds','Array','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0'):
        print("Array setting failed!")

    sources = getParam('ferolSources','Array','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    newSources = []
    for source in sources:
        items = []
        for item in source:
            if item[0] == 'fedId':
                items.append((item[0],item[1],item[2]+10))
            else:
                items.append(item)
        newSources.append(items)
    setParam('ferolSources','Array',newSources,'kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    if newSources != getParam('ferolSources','Array','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0'):
        print("Tuple setting failed!")

    sources = getParam('ferolSources','Array','kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
    newSources = []
    for source in sources:
        items = []
        for item in source:
            if item[0] == 'dummyFedSize':
                items.append((item[0],item[1],'1056'))
        newSources.append(items)
    setParam('ferolSources','Array',newSources,'kvm-s3562-1-ip151-39.cms','65440','','evb::EVM','0')
