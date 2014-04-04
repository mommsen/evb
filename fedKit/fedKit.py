#!/usr/bin/python

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

print("Running. Point your browser to "+fedKitConfig.xdaqProcesses[-1].getURL())

ans = "m"
while ans.lower() != 'q':
    if ans.lower() == 'm':
        print("""
m   display this Menu
q   stop the run and Quit
""")
    ans = raw_input("=> ")


for xdaq in fedKitConfig.xdaqProcesses:
    xdaq.kill()

