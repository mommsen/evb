#!/usr/bin/python

import argparse
import os
import socket
import subprocess
import sys
import threading
import SocketServer


class xdaqThread(threading.Thread):

    def __init__(self,xdaqPort,logFile=None):
        threading.Thread.__init__(self)
        self.logFile = logFile
        self._process = None
        xdaqRoot = os.environ["XDAQ_ROOT"]
        os.environ["XDAQ_DOCUMENT_ROOT"] = xdaqRoot+"/htdocs"
        self._xdaqCommand = [xdaqRoot+"/bin/xdaq.exe","-e "+xdaqRoot+"/etc/default.profile","-p "+str(xdaqPort)]


    def run(self):
        if self.logFile:
            with open(self.logFile,"w") as logfile:
                self._process = subprocess.Popen(self._xdaqCommand,stdout=logfile,stderr=logfile)
                self._process.wait()
        else:
            self._process = subprocess.Popen(self._xdaqCommand)
            self._process.wait()


    def kill(self):
        if self._process:
            self._process.kill()


class TCPServer(SocketServer.TCPServer):
    def __init__(self, server_address, RequestHandlerClass, bind_and_activate=True, logFile=None):
        self.logFile = logFile
        SocketServer.TCPServer.__init__(self, server_address, RequestHandlerClass, bind_and_activate=bind_and_activate)


class xdaqLauncher(SocketServer.BaseRequestHandler):

    threads = {}

    def __init__(self, request, client_address, server):
        SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)


    def handle(self):
        commands = {
            "startXDAQ":self.startXDAQ,
            "stopXDAQ":self.stopXDAQ,
            "stopLauncher":self.stopLauncher
            }

        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        #print("Received '"+data+"' from "+self.client_address[0])
        sys.stdout.flush()
        try:
            (command,port) = data.split(':',2)
            try:
                xdaqPort = int(port)
            except (TypeError,ValueError):
                self.request.sendall("Received an invalid port number: '"+port+"'")
                return
        except ValueError:
            command = data
            port = None
        try:
            commands[command](port)
        except KeyError:
            self.request.sendall("Received unknown command '"+command+"'")


    def startXDAQ(self,port):
        if port is None:
            self.request.sendall("Please specify a port number")
            return
        if port in xdaqLauncher.threads and xdaqLauncher.threads[port].is_alive():
            self.request.sendall("There is already a XDAQ process running on port "+str(port))
            return
        try:
            thread = xdaqThread(port,self.server.logFile)
            thread.start()
        except KeyError:
            self.request.sendall("Please specify XDAQ_ROOT")
            return
        xdaqLauncher.threads[port] = thread
        self.request.sendall("Started XDAQ on port "+str(port))


    def killProcess(self,port):
        thread = xdaqLauncher.threads[port]
        if thread.is_alive():
            thread.kill()
            thread.join()
            return "Killed XDAQ process on port "+str(port)
        else:
            return "XDAQ process on port "+str(port)+" has already died: "+str(thread._process.returncode)


    def stopXDAQ(self,port):
        response = ""
        if port is None:
            for port in xdaqLauncher.threads.keys():
                response += self.killProcess(port)+"\n"
        else:
            try:
                response = self.killProcess(port)
            except KeyError:
                self.request.sendall("There is no XDAQ processes on port "+str(port))
        if response == "":
            self.request.sendall("There are no running XDAQ processes")
        else:
            self.request.sendall(response)


    def stopLauncher(self,port):
        response = ""
        for port in xdaqLauncher.threads.keys():
            response += self.killProcess(port)+"\n"
        response += "Terminating launcher"
        self.request.sendall(response)
        threading.Thread(target = self.server.shutdown).start()


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-l","--logFile",help="redirect stdout and stderr to log file")
    parser.add_argument("port",type=int)
    args = vars( parser.parse_args() )
    hostname = socket.gethostbyaddr(socket.gethostname())[0]
    server = TCPServer((hostname,args['port']), xdaqLauncher, logFile=args['logFile'])
    server.serve_forever()
