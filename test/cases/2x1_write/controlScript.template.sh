#!/bin/sh

# Cleanup
testDir=/tmp/rubuilder_test
rm -rf $testDir

# Launch executive processes
sendCmdToLauncher EVM0_SOAP_HOST_NAME EVM0_LAUNCHER_PORT STARTXDAQEVM0_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ RU0_SOAP_HOST_NAME RU0_SOAP_PORT 5
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
sendCmdToExecutive EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml

# Configure and enable ptatcp
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Configure
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT pt::atcp::PeerTransportATCP 0 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::atcp::PeerTransportATCP 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::atcp::PeerTransportATCP 2 Enable

# Configure all applications
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir

sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure

sleep 1

runNumber=`date "+%s"`

#Enable EVM
setParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 Enable

#Enable RU
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 Enable

#Enable BUs
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

echo "Sending data for 5 seconds"
sleep 5

superFragmentSizeEVM=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt`
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne 2048 ]]
then
  echo "Test failed: expected 2048"
  exit 1
fi

superFragmentSizeRU0=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt`
echo "RU0 superFragmentSize: $superFragmentSizeRU0"
if [[ $superFragmentSizeRU0 -ne 16384 ]]
then
  echo "Test failed: expected 16384"
  exit 1
fi

eventRateEVM=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt`
echo "EVM eventRate: $eventRateEVM"
if [[ $eventRateEVM -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

nbEventsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedInt`
echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

eventSizeBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventSize xsd:unsignedInt`
echo "BU0 eventSize: $eventSizeBU0"
if [[ $eventSizeBU0 -ne 18432 ]]
then
  echo "Test failed: expected 18432"
  exit 1
fi

#Stop EVM
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 Stop

sleep 2

state=`getParam EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT evb::EVM 0 stateName xsd:string`
echo "EVM state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

#Stop RU
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 Stop

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::RU 0 stateName xsd:string`
echo "RU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

#Stop BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Stop

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string`
echo "BU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

ls -l $testDir/BU-000/run$runNumber

if [[ -x $testDir/BU-000/run$runNumber/open ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/open exists"
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/rawData.jsd ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/rawData.jsd does not exist"
    exit 1
fi

rawFiles=`ls $testDir/BU-000/run$runNumber/ls*raw | wc -l`
jsnFiles=`ls $testDir/BU-000/run$runNumber/ls*jsn | wc -l`
if [[ $rawFiles != $jsnFiles ]]
then
    echo "Test failed: found $rawFiles raw files, but $jsnFiles JSON files."
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/EoLS.jsd ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoLS.jsd does not exist"
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/EoLS_0001.jsn ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoLS_0000.jsn does not exist"
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/EoR.jsd ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoR.jsd does not exist"
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/EoR_$runNumber.jsn ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoR_$runNumber.jsn does not exist"
    exit 1
fi

echo "Test completed successfully"
exit 0
