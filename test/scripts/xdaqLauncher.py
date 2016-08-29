#!/usr/bin/env python

import argparse
import glob
import os
import shutil
import socket
import subprocess
import sys
import threading
import SocketServer


class xdaqThread(threading.Thread):

    def __init__(self,xdaqPort,logFile,useNuma):
        threading.Thread.__init__(self)
        self.logFile = logFile
        self._process = None
        xdaqRoot = os.environ["XDAQ_ROOT"]
        os.environ["XDAQ_DOCUMENT_ROOT"] = xdaqRoot+"/htdocs"
        os.environ["LD_LIBRARY_PATH"] = xdaqRoot+"/lib"
        hostname = socket.gethostname()
        numaCtl = []
        if useNuma:
            if 'ru-' in hostname:
                numaCtl = ['/usr/bin/numactl','--cpunodebind=1','--membind=1']
            elif 'bu-' in hostname:
                numaCtl = ['/usr/bin/numactl','--physcpubind=10,12,14,26,28,30','--membind=1']

        self._xdaqCommand = numaCtl + [xdaqRoot+"/bin/xdaq.exe","-e"+xdaqRoot+"/etc/default.profile","-p"+str(xdaqPort)]


    def run(self):
        if self.logFile:
            with open(self.logFile,"w",0) as logfile:
                self._process = subprocess.Popen(self._xdaqCommand,stdout=logfile,stderr=subprocess.STDOUT,close_fds=True)
        else:
            self._process = subprocess.Popen(self._xdaqCommand)
        self._process.wait()


    def kill(self):
        if self._process:
            self._process.kill()


class TCPServer(SocketServer.TCPServer):
    def __init__(self, server_address, RequestHandlerClass, logDir, useNuma):
        SocketServer.TCPServer.allow_reuse_address = True
        SocketServer.TCPServer.__init__(self, server_address, RequestHandlerClass)
        self.logDir = logDir
        self.useNuma = useNuma


class xdaqLauncher(SocketServer.BaseRequestHandler):

    threads = {}

    def __init__(self, request, client_address, server):
        SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)


    def handle(self):
        commands = {
            "startXDAQ":self.startXDAQ,
            "stopXDAQ":self.stopXDAQ,
            "stopLauncher":self.stopLauncher,
            "getFiles":self.getFiles
            }
        args = None

        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        #print("Received '"+data+"' from "+self.client_address[0])
        sys.stdout.flush()
        try:
            (command,portStr) = data.split(':',2)
            try:
                (portStr,args) = portStr.split(' ',2)
            except ValueError:
                pass
            try:
                port = int(portStr)
            except (TypeError,ValueError):
                self.request.sendall("Received an invalid port number: '"+portStr+"'")
                return
        except ValueError:
            command = data
            port = None
        try:
            commands[command](port,args)
        except KeyError:
            self.request.sendall("Received unknown command '"+command+"'")


    def cleanTempFiles(self):
        try:
            for file in glob.glob("/tmp/dump_*txt"):
                os.remove(file)
            shutil.rmtree("/tmp/evb_test")
        except OSError:
            pass


    def getLogFile(self,testname):
        if self.server.logDir:
            return self.server.logDir+"/"+testname+"-"+socket.gethostname()+".log"
        else:
            return None


    def startXDAQ(self,port,testname):
        if self.server.logDir and testname is None:
            self.request.sendall("Please specify a testname")
            return
        if port is None:
            self.request.sendall("Please specify a port number")
            return
        if port in xdaqLauncher.threads and xdaqLauncher.threads[port].is_alive():
            self.request.sendall("There is already a XDAQ process running on port "+str(port))
            return
        self.cleanTempFiles()
        try:
            thread = xdaqThread(port,self.getLogFile(testname),self.server.useNuma)
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


    def stopXDAQ(self,port,args):
        response = ""
        self.cleanTempFiles()
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


    def stopLauncher(self,port,args):
        response = ""
        for port in xdaqLauncher.threads.keys():
            response += self.killProcess(port)+"\n"
        response += "Terminating launcher"
        self.request.sendall(response)
        threading.Thread(target = self.server.shutdown).start()


    def getFiles(self,port,dir):
        if dir is None:
            self.request.sendall("Please specify a directory")
            return
        self.request.sendall( str(os.listdir(dir)) )


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-l","--logDir",help="redirect stdout and stderr to log file")
    parser.add_argument("-n","--useNuma",action='store_true',help="use numactl to start xdaq.exe")
    parser.add_argument("port",type=int)
    args = vars( parser.parse_args() )
    hostname = socket.gethostbyaddr(socket.gethostname())[0]
    if args['useNuma'] and any(x in hostname for x in ('ru-','bu-')):
        useNuma = True
    else:
        useNuma = False
    server = TCPServer((hostname,args['port']), xdaqLauncher, logDir=args['logDir'], useNuma=useNuma)
    server.serve_forever()
