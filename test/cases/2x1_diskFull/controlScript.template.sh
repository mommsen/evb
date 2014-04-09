#!/bin/sh

source EVB_TESTER_HOME/cases/helpers.sh

# Cleanup
testDir=/tmp/evb_test
rm -rf $testDir
mkdir -p $testDir
echo "dummy HLT menu for EvB test" >> $testDir/HltConfig.py
echo "slc6_amd64_gcc472" >> $testDir/SCRAM_ARCH
echo "cmssw_noxdaq" >> $testDir/CMSSW_VERSION

# Launch executive processes
sendCmdToLauncher FEROL0_SOAP_HOST_NAME FEROL0_LAUNCHER_PORT STARTXDAQFEROL0_SOAP_PORT
sendCmdToLauncher FEROL1_SOAP_HOST_NAME FEROL1_LAUNCHER_PORT STARTXDAQFEROL1_SOAP_PORT
sendCmdToLauncher FEROL2_SOAP_HOST_NAME FEROL2_LAUNCHER_PORT STARTXDAQFEROL2_SOAP_PORT
sendCmdToLauncher FEROL3_SOAP_HOST_NAME FEROL3_LAUNCHER_PORT STARTXDAQFEROL3_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

# Check that executives are listening
if ! webPingXDAQ FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
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
sendCmdToExecutive FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml

# Configure and enable pt
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Enable
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Enable

# Configure all applications
diskUsage=$(df $testDir |sed -nre 's/.*\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)\s+([0-9]+)%.*/\2\/\1/p')
minDiskUsage=$(bc -l <<< "$diskUsage")
maxDiskUsage=$(bc -l <<< "$diskUsage + 0.001")
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataLowWaterMark double $minDiskUsage
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataHighWaterMark double $maxDiskUsage
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir

sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure

sleep 1

runNumber=$(date "+%s")

#Enable EVM
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RU
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 Enable

#Enable BU
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir/"
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

#Enable FEROLs
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Enable

echo "Sending data..."
sleep 5
while [[ $(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt) -gt 0 ]]
do
    sleep 5
done

sleep 10

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "EVM state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

rm -f $testDir/run$runNumber/*.raw
sleep 5

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "EVM state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

eventRateEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt)
echo "EVM eventRate: $eventRateEVM"
if [[ $eventRateEVM -lt 500 ]]
then
  echo "Test failed"
  exit 1
fi

echo "Test launched successfully"
exit 0
