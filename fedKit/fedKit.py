#!/usr/bin/python

import httplib
import time

import FedKitConfig
import XDAQprocess



print("Welcome to the optical FEDkit")
print("=============================")

fedKitConfig = FedKitConfig.FedKitConfig()

print("Starting run...")

for xdaq in fedKitConfig.xdaqProcesses:
    xdaq.create(fedKitConfig.configFilePath)

time.sleep(2)

for xdaq in fedKitConfig.xdaqProcesses[::-1]:
    xdaq.start()

url=fedKitConfig.xdaqProcesses[-1].getURL()
print("Running. Point your browser to http://"+url)

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
    xdaq.kill()

