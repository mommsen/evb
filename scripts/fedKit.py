#!/usr/bin/python

import getopt
import httplib
import os
import sys
import time

import FedKitConfig
import XDAQprocess


def usage():
    print """
fedKit.py [--config <configfile>] [--nbEvents <count>] [--reconfigure] [--dummyFerol]
          -h --help:         print this message and exit
          -c --config:       path to a config file (default ./fedKit.cfg)
          -n --nbEvents:     kill the processes after reaching the specified number of events
          -r --reconfigure:  ask for all configuration options (default takes them from configfile if it exists)
          -d --dummyFerol:   use a software emulator of the FEROL (default uses the real h/w FEROL)
    """


def main(argv):
    os.environ["XDAQ_ROOT"] = "/opt/xdaq"
    os.environ["XDAQ_DOCUMENT_ROOT"] = "/opt/xdaq/htdocs"
    try:
        os.environ["LD_LIBRARY_PATH"] = "/opt/xdaq/lib:"+os.environ["LD_LIBRARY_PATH"]
    except KeyError:
        os.environ["LD_LIBRARY_PATH"] = "/opt/xdaq/lib"
    try:
        os.environ["PATH"] = "/opt/xdaq/bin:"+os.environ["PATH"]
    except KeyError:
        os.environ["PATH"] = "/opt/xdaq/bin"

    configfile = "fedKit.cfg"
    reconfigure = False;
    useDummyFerol = False;
    haltAfterNbEvents = 0;

    try:
        opts,args = getopt.getopt(argv,"c:n:rd",["config=","nbEvents=","reconfigure","dummyFerol"])
    except getopt.GetoptError:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            usage()
            sys.exit()
        elif opt in ("-c", "--config"):
            configfile = arg
        elif opt in ("-n", "--nbEvents"):
            haltAfterNbEvents = int(arg)
        elif opt in ("-r", "--reconfigure"):
            reconfigure = True
        elif opt in ("-d", "--dummyFerol"):
            useDummyFerol = True

    print("Welcome to the optical FEDkit")
    print("=============================")

    reconfigure = reconfigure or not os.path.isfile(configfile)
    fedKitConfig = FedKitConfig.FedKitConfig(configfile,reconfigure,useDummyFerol)

    print("Starting run...")

    for xdaq in fedKitConfig.xdaqProcesses:
        xdaq.create(fedKitConfig.configFilePath)

    time.sleep(2)

    for xdaq in fedKitConfig.xdaqProcesses:
        xdaq.waitForHalted()
    for xdaq in fedKitConfig.xdaqProcesses[::-1]:
        xdaq.configure()
    for xdaq in fedKitConfig.xdaqProcesses:
        xdaq.waitForReady()
    for xdaq in fedKitConfig.xdaqProcesses[::-1]:
        xdaq.enable()

    url=fedKitConfig.xdaqProcesses[-1].getURL()
    print("Running. Point your browser to http://"+url)

    if haltAfterNbEvents:
        nbEvents = 0
        while nbEvents < haltAfterNbEvents:
            nbEvents = int( fedKitConfig.xdaqProcesses[-1].getParam("eventCount","unsignedLong","evb::EVM",0) )

        nbEvents = int( fedKitConfig.xdaqProcesses[-1].getParam("eventCount","unsignedLong","evb::EVM",0) )
        print("Terminating after building "+str(nbEvents)+" events")

    else:
        ans = "m"
        while ans.lower() != 'q':

            if ans.lower() == 'm':
                print("""
m   display this Menu
f#  dump the next # FED fragments incl DAQ headers (default 1)
e#  dump the next # Events with FED data only (default 1)
q   stop the run and Quit
                """)

            if ans and ans.lower()[0] == 'f':
                try:
                    count = int(ans[1:])
                except (ValueError,NameError,TypeError):
                    count = 1
                print("Dumping "+str(count)+" FED fragments to /tmp")
                conn = httplib.HTTPConnection(url)
                urn = fedKitConfig.xdaqProcesses[-1].getURN("evb::EVM",0)
                urn += "/writeNextFragmentsToFile?count="+str(count)
                conn.request("GET",urn)

            if ans and ans.lower()[0] == 'e':
                try:
                    count = int(ans[1:])
                except (ValueError,NameError,TypeError):
                    count = 1
                print("Dumping "+str(count)+" events to /tmp")
                conn = httplib.HTTPConnection(url)
                urn = fedKitConfig.xdaqProcesses[-1].getURN("evb::BU",0)
                urn += "/writeNextEventsToFile?count="+str(count)
                conn.request("GET",urn)

            ans = raw_input("=> ")

        for xdaq in fedKitConfig.xdaqProcesses:
            xdaq.stop()


if __name__ == "__main__":
    main(sys.argv[1:])
