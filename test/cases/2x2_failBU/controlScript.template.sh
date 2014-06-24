#!/bin/sh

source EVB_TESTER_HOME/cases/helpers.sh

# Cleanup
testDir=/tmp/evb_test/ramdisk
rm -rf $testDir
mkdir -p $testDir
echo "dummy HLT menu for EvB test" >> $testDir/HltConfig.py
echo "slc6_amd64_gcc472" >> $testDir/SCRAM_ARCH
echo "cmssw_noxdaq" >> $testDir/CMSSW_VERSION

# Launch executive processes
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
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
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME BU1_SOAP_PORT configure.cmd.xml

# Configure and enable pt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT pt::utcp::Application 3 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT pt::utcp::Application 3 Enable

# Configure all applications
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir/BU0
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 rawDataDir string $testDir/BU1
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir/BU0
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 metaDataDir string $testDir/BU1

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Configure

sleep 1

runNumber=$(date "+%s")

#Enable EVM
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RU
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable

#Enable BUs
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir/"
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 hltParameterSetURL string "file://$testDir/"
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Enable

echo "Sending data for 12 seconds"
sleep 12

superFragmentSizeEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt)
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne 2048 ]]
then
  echo "Test failed: expected 2048"
  exit 1
fi

superFragmentSizeRU1=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt)
echo "RU1 superFragmentSize: $superFragmentSizeRU1"
if [[ $superFragmentSizeRU1 -ne 24576 ]]
then
  echo "Test failed: expected 24576"
  exit 1
fi

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"
if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

eventSizeBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventSize xsd:unsignedInt)
echo "BU0 eventSize: $eventSizeBU0"
if [[ $eventSizeBU0 -ne 26624 ]]
then
  echo "Test failed: expected 26624"
  exit 1
fi

eventSizeBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 eventSize xsd:unsignedInt)
echo "BU1 eventSize: $eventSizeBU1"
if [[ $eventSizeBU1 -ne 26624 ]]
then
  echo "Test failed: expected 26624"
  exit 1
fi

#Fail BU 0
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Fail

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Failed" ]]
then
  echo "Test failed"
  exit 1
fi

checkBuDir $testDir/BU0/run$runNumber 28672 0


#Stop EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Stop

sleep 2

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "EVM state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

#Stop RU
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Stop

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

#Stop BU
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Stop
sleep 5

state=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

checkBuDir $testDir/BU1/run$runNumber 28672 0

echo "Test completed successfully"
exit 0
