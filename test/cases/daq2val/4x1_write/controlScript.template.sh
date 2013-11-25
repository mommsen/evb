#!/bin/sh

# Launch executive processes
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher RU2_SOAP_HOST_NAME RU2_LAUNCHER_PORT STARTXDAQRU2_SOAP_PORT
sendCmdToLauncher RU3_SOAP_HOST_NAME RU3_LAUNCHER_PORT STARTXDAQRU3_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU1_SOAP_HOST_NAME RU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU2_SOAP_HOST_NAME RU2_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU3_SOAP_HOST_NAME RU3_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU2_SOAP_HOST_NAME RU2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU3_SOAP_HOST_NAME RU3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml

# Set fragment size
#fragmentSize=16384
fragmentSize=15632
#fragmentSize=1024
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 dummyFedSize unsignedInt $fragmentSize
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 dummyFedSize unsignedInt $fragmentSize
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 dummyFedSize unsignedInt $fragmentSize
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 dummyFedSize unsignedInt $fragmentSize

testCRC=false
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 computeCRC boolean $testCRC
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 computeCRC boolean $testCRC
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 computeCRC boolean $testCRC
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 computeCRC boolean $testCRC
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 checkCRC boolean $testCRC

# Configure all applications
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure

sleep 1

#runNumber=`date "+%l%m%S"`
#runNumber=`date "+%H%M%S"`
seconds=`date "+%s"`
runNumber=${seconds:(-6)}
runNumber=${runNumber/#0/1}
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 runNumber unsignedInt $runNumber
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber

#Enable EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

echo "Sending data... Hit any key to stop the run"
read -n 1 -s

#Stop EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Stop
sleep 2

#Stop RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Stop
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Stop
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Stop
sleep 3

#Stop BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Stop
sleep 3

echo "Test completed successfully"
exit 0
