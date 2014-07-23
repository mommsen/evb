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

# Configure all executives
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml

# Configure and enable pt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Enable

# Configure all applications
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure

sleep 1

runNumber=$(date "+%s")

#Enable EVM
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RU
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable

#Enable BU
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir/"
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

echo "Sending data for 15 seconds"
sleep 15

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

eventSizeBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventSize xsd:unsignedInt)
echo "BU0 eventSize: $eventSizeBU0"
if [[ $eventSizeBU0 -ne 26624 ]]
then
  echo "Test failed: expected 26624"
  exit 1
fi

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

echo "Wait for lumi section timeouts"
sleep 15

#Stop RU
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Stop

#Stop BU
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Stop

sleep 2

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

eventCountEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventCount xsd:unsignedLong)
echo "EVM eventCount: $eventCountEVM"
if [[ $eventCountEVM -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

if [[ $eventCountEVM -ne $nbEventsBuiltBU0 ]]
then
  echo "Test failed: EVM counted $eventCountEVM events, while BU built $nbEventsBuiltBU0 events"
  exit 1
fi

checkBuDir $testDir/run$runNumber 28672 1

echo "Test completed successfully"
exit 0
