#!/usr/bin/env python

import socket, sys
from xml.dom import minidom

#----------------------------------------------------------------------

# we need a small web server to respond to webping

import SocketServer, SimpleHTTPServer


# note that there may be a new DummyServer instance for
# each request. So we keep the state outside the object.
#
# first key is application URN
# second key is parameter name
xdaq_params = {}

class DummyHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def __init__(self, socket, peer_addr, tcp_server, debug):
        # for some unknown reason we have to set this attribute 
        # before calling the parent constructor
        self.debug = debug
        SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self, socket, peer_addr, tcp_server)

    def log_message(self, format, *args):
        if self.debug:
            SimpleHTTPServer.SimpleHTTPRequestHandler.log_message(self, format, *args)


    def do_GET(self):
        # this is called when a new connection is opened to this server

        # write response
        self.protocol_version = 'HTTP/1.1'
        self.send_response(200, 'OK')
        self.send_header('Content-type', 'application/text')
        self.end_headers()
        self.wfile.write(bytes("HELLO"))
        return


    def respond_soap(self, response):
        # respond with the given SOAP message
        self.send_response(200, 'OK')
        self.send_header('Content-type', 'text/xml')
        self.send_header("Content-Description", "SOAP Message")
        self.end_headers()

        self.wfile.write(bytes("""<?xml version="1.0" encoding="UTF-8"?>
<soap-env:Envelope
soap-env:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"
xmlns:soap-env="http://schemas.xmlsoap.org/soap/envelope/"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xmlns:xsd="http://www.w3.org/2001/XMLSchema"
xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
xmlns:xdaq="urn:xdaq-soap:3.0">
<soap-env:Header/>
<soap-env:Body>""" + response + """</soap-env:Body>
</soap-env:Envelope>
"""
                               ))

        
    #----------------------------------------

    def handle_configure(self, application):
        # note that configure typically spans more than one application

        if not application is None:
            xdaq_params.setdefault(application,{})['stateName'] = 'Configured'

        self.respond_soap("<xdaq:ConfigureResponse/>")

    #----------------------------------------

    def handle_param_get(self, application, param_name):

        if self.debug:
            self.log_message("ParamGet for parameter " + param_name + " of application " + str(application))

        # special treatment for stateName: if it is not present, assume 'Configured'
        value = xdaq_params.setdefault(application, {}).get(param_name, None)
        if param_name == 'stateName' and value == None:

            # default states vary by application
            if application.startswith('evb::RU') or application.startswith("ferol::FerolController") or application.startswith("evb::EVM") or application.startswith("evb::BU"):
                # after Configure the applications seem to go to 'Ready' ?
                value = 'Halted'
            else:
                value = 'Configured'

            xdaq_params[application][param_name] = value

        elif param_name == 'eventRate' and value == None:
            # return a default event rate
            value = 100000
            xdaq_params[application][param_name] = value

        elif param_name == 'superFragmentSize' and value == None:
            value = 16000
            xdaq_params[application][param_name] = value

        value = str(value)
        
        self.respond_soap('<p:%s xmlns:p="%s">%s</p:%s>' % (param_name, application, value, param_name))
    #----------------------------------------

    def handle_param_set(self, application, param_name, param_value):
        value = xdaq_params.setdefault(application, {})[param_name] = param_value
        
        self.respond_soap("<xdaq:ParameterSetResponse/>")

    #----------------------------------------

    def handle_connect(self, application):
        # put all known applications into 'Enabled' state ?
        if self.debug:
            self.log_message("connect, known applications=" + ", ".join(xdaq_params.keys()))


        xdaq_params.setdefault(application, {})['stateName'] = 'Enabled'
        self.respond_soap("<xdaq:connectResponse/>")

    #----------------------------------------

    def handle_enable(self, application):

        xdaq_params.setdefault(application, {})['stateName'] = 'Enabled'
        self.respond_soap("<xdaq:EnableResponse/>")

    #----------------------------------------

    def handle_halt(self, application):
        xdaq_params.setdefault(application, {})['stateName'] = 'Halted'
        self.respond_soap("<xdaq:HaltResponse/>")

    #----------------------------------------

    def do_POST(self):

        if 'SOAPAction' in self.headers:

            # example for this field: urn:xdaq-application:class=pt::ibv::Application,instance=2
            soap_action = self.headers.get('SOAPAction')

            if soap_action.startswith("urn:xdaq-application:class="):
                application = soap_action.split('=',1)[1]
            else:
                application = None

            # this is a SOAP message
            # request will typically be an XML SOAP message 
            request = self.rfile.read(int(self.headers.get('content-length')))
            # self.log_message("soap request=" + str(request))

            request = minidom.parseString(request)

            # get the body part
            body = request.getElementsByTagName("soap-env:Body")
            assert len(body) == 1
            body = body[0]
            # self.log_message("body=" + str(body))

            # get XDAQ command
            commands = [ node for node in body.childNodes if node.nodeType == node.ELEMENT_NODE ]
            assert len(commands) == 1
            command = commands[0]

            if self.debug:
                self.log_message("soap command: " + command.localName + " for application " + str(application))

            if (command.namespaceURI, command.localName) == ('urn:xdaq-soap:3.0', 'Configure'):
                self.handle_configure(application)
                return

            if (command.namespaceURI, command.localName) == ('urn:xdaq-soap:3.0', 'connect'):
                self.handle_connect(application)
                return 

            if (command.namespaceURI, command.localName) == ('urn:xdaq-soap:3.0', 'Enable'):
                self.handle_enable(application)
                return 

            if (command.namespaceURI, command.localName) == ('urn:xdaq-soap:3.0', 'Halt'):
                self.handle_halt(application)
                return 

            if command.namespaceURI == 'urn:xdaq-soap:3.0' and command.localName in ('ParameterGet', 'ParameterSet'):
                # expect a properties tag
                properties = command.getElementsByTagName("p:properties")
                assert len(properties) == 1
                properties = properties[0]

                # find out for which application class the parameters are requested
                # example: "urn:xdaq-application:evb::RU"
                # application = properties.namespaceURI

                param_nodes = [ node for node in properties.childNodes if node.nodeType == node.ELEMENT_NODE ]

                assert len(param_nodes) == 1
                param_node = param_nodes[0]

                if command.localName == 'ParameterGet':
                    self.handle_param_get(application, param_node.localName)
                    return
                else:
                    # ParameterSet: extract the value
                    value = " ".join(t.nodeValue for t in param_node.childNodes if t.nodeType == t.TEXT_NODE)

                    if self.debug:
                        self.log_message("ParamSet: value=" + value)

                    self.handle_param_set(application, param_node.localName, value)
                    return

            # unsupported request
            self.log_message("unsupported command " + command.localName)
            self.log_message("SOAP message of unsupported command " + request.toprettyxml())
            self.log_message("headers of unsupported command " + str(self.headers))
            self.send_response(404, 'Not Found')
            self.end_headers()
            return



        self.do_GET()


#----------------------------------------------------------------------
# main
#----------------------------------------------------------------------
if __name__ == '__main__':

    from argparse import ArgumentParser


    parser = ArgumentParser(description = """
    dummy server to develop new functionality in the xdaqLauncher
    without actually running any xdaq processes
    """
    )
    parser.add_argument("-p",
                        dest = "port",
                        type = int,
                        required = True,
                        help="port to listen on",
                        metavar = "port")

    parser.add_argument("--debug",
                        default = False,
                        action = "store_true",
                        help="enable debug log messages",
                        )

    options = parser.parse_args()

    hostname = socket.gethostname()

    def factory(socket, peer_addr, tcp_server):
        return DummyHandler(socket, peer_addr, tcp_server, options.debug)

    server = SocketServer.TCPServer((hostname, options.port), factory,
                                    bind_and_activate = False)
    server.allow_reuse_address = True
    server.server_bind()
    server.server_activate()

    server.serve_forever()

