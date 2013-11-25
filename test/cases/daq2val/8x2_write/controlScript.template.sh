#!/bin/sh

# Launch executive processes
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher RU2_SOAP_HOST_NAME RU2_LAUNCHER_PORT STARTXDAQRU2_SOAP_PORT
sendCmdToLauncher RU3_SOAP_HOST_NAME RU3_LAUNCHER_PORT STARTXDAQRU3_SOAP_PORT
sendCmdToLauncher RU4_SOAP_HOST_NAME RU4_LAUNCHER_PORT STARTXDAQRU4_SOAP_PORT
sendCmdToLauncher RU5_SOAP_HOST_NAME RU5_LAUNCHER_PORT STARTXDAQRU5_SOAP_PORT
sendCmdToLauncher RU6_SOAP_HOST_NAME RU6_LAUNCHER_PORT STARTXDAQRU6_SOAP_PORT
sendCmdToLauncher RU7_SOAP_HOST_NAME RU7_LAUNCHER_PORT STARTXDAQRU7_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher BU1_SOAP_HOST_NAME BU1_LAUNCHER_PORT STARTXDAQBU1_SOAP_PORT

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
if ! webPingXDAQ RU4_SOAP_HOST_NAME RU4_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU5_SOAP_HOST_NAME RU5_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU6_SOAP_HOST_NAME RU6_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU7_SOAP_HOST_NAME RU7_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU0_SOAP_HOST_NAME BU0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU1_SOAP_HOST_NAME BU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU2_SOAP_HOST_NAME RU2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU3_SOAP_HOST_NAME RU3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU4_SOAP_HOST_NAME RU4_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU5_SOAP_HOST_NAME RU5_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU6_SOAP_HOST_NAME RU6_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU7_SOAP_HOST_NAME RU7_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME BU1_SOAP_PORT configure.cmd.xml

# Set fragment size
fragmentSize=7816
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 dummyFedSize unsignedInt $fragmentSize
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 dummyFedSize unsignedInt $fragmentSize
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 dummyFedSize unsignedInt $fragmentSize
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 dummyFedSize unsignedInt $fragmentSize
setParam RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 dummyFedSize unsignedInt $fragmentSize
setParam RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 dummyFedSize unsignedInt $fragmentSize
setParam RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 dummyFedSize unsignedInt $fragmentSize
setParam RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 dummyFedSize unsignedInt $fragmentSize

testCRC=false
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 computeCRC boolean $testCRC
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 computeCRC boolean $testCRC
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 computeCRC boolean $testCRC
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 computeCRC boolean $testCRC
setParam RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 computeCRC boolean $testCRC
setParam RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 computeCRC boolean $testCRC
setParam RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 computeCRC boolean $testCRC
setParam RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 computeCRC boolean $testCRC
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 checkCRC boolean $testCRC
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 checkCRC boolean $testCRC

# Configure all applications
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Configure
sendSimpleCmdToApp RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 Configure
sendSimpleCmdToApp RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 Configure
sendSimpleCmdToApp RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 Configure
sendSimpleCmdToApp RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Configure

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
setParam RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 runNumber unsignedInt $runNumber
setParam RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 runNumber unsignedInt $runNumber
setParam RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 runNumber unsignedInt $runNumber
setParam RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 runNumber unsignedInt $runNumber

#Enable EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Enable
sendSimpleCmdToApp RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 Enable
sendSimpleCmdToApp RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 Enable
sendSimpleCmdToApp RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 Enable
sendSimpleCmdToApp RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Enable

echo "Sending data... Hit any key to stop the run"
read -n 1 -s

#Stop EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Stop
sleep 2

#Stop RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Stop
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Stop
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Stop
sendSimpleCmdToApp RU4_SOAP_HOST_NAME RU4_SOAP_PORT evb::RU 3 Stop
sendSimpleCmdToApp RU5_SOAP_HOST_NAME RU5_SOAP_PORT evb::RU 4 Stop
sendSimpleCmdToApp RU6_SOAP_HOST_NAME RU6_SOAP_PORT evb::RU 5 Stop
sendSimpleCmdToApp RU7_SOAP_HOST_NAME RU7_SOAP_PORT evb::RU 6 Stop
sleep 3

#Stop BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Stop
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Stop
sleep 3

echo "Test completed successfully"
exit 0
