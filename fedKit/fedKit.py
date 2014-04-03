#!/usr/bin/python

import time

import FedKitConfig
import XDAQprocess



print("Welcome to the optical FEDkit")
print("=============================")

fedKitConfig = FedKitConfig.FedKitConfig()

for xdaq in fedKitConfig.xdaqProcesses:
    xdaq.create(fedKitConfig.configFilePath)

time.sleep(2)

for xdaq in fedKitConfig.xdaqProcesses[::-1]:
    xdaq.start()

time.sleep(60)

for xdaq in fedKitConfig.xdaqProcesses:
    xdaq.kill()

