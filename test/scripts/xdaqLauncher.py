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
    def __init__(self, server_address, RequestHandlerClass, bind_and_activate=True, logDir=None):
        self.logDir = logDir
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
        testname = None

        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        #print("Received '"+data+"' from "+self.client_address[0])
        sys.stdout.flush()
        try:
            (command,port) = data.split(':',2)
            if command == "startXDAQ":
                try:
                    (port,testname) = port.split(' ',2)
                except ValueError:
                    if self.logDir:
                        self.request.sendall("Please specify a testname")
                        return
                    else:
                        pass
            try:
                xdaqPort = int(port)
            except (TypeError,ValueError):
                self.request.sendall("Received an invalid port number: '"+port+"'")
                return
        except ValueError:
            command = data
            port = None
        try:
            commands[command](port,testname)
        except KeyError:
            self.request.sendall("Received unknown command '"+command+"'")


    def getLogFile(self,testname):
        if self.server.logDir:
            return self.server.logDir+"/"+testname+"-"+socket.gethostname()+".log"
        else:
            return None


    def startXDAQ(self,port,testname):
        if port is None:
            self.request.sendall("Please specify a port number")
            return
        if port in xdaqLauncher.threads and xdaqLauncher.threads[port].is_alive():
            self.request.sendall("There is already a XDAQ process running on port "+str(port))
            return
        try:
            thread = xdaqThread(port,self.getLogFile(testname))
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


    def stopXDAQ(self,port,testname):
        response = ""
        if port is None:
            for port in xdaqLauncher.threads.keys():
                response += self.killProcess(port)+"\n"
        else:
            try:
                response = self.killProcess(port)
            except KeyError:
                self.request.sendall("There is no XDAQ processes on port "+str(port))
                return
        if response == "":
            self.request.sendall("There are no running XDAQ processes")
        else:
            self.request.sendall(response)


    def stopLauncher(self,port,testname):
        response = ""
        for port in xdaqLauncher.threads.keys():
            response += self.killProcess(port)+"\n"
        response += "Terminating launcher"
        self.request.sendall(response)
        threading.Thread(target = self.server.shutdown).start()


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-l","--logDir",help="redirect stdout and stderr to log file")
    parser.add_argument("port",type=int)
    args = vars( parser.parse_args() )
    hostname = socket.gethostbyaddr(socket.gethostname())[0]
    server = TCPServer((hostname,args['port']), xdaqLauncher, logDir=args['logDir'])
    server.serve_forever()
