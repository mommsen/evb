#!/bin/sh

function sendCmdToEVM
{
  response=$(sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 $1)
  if [[ ! $response =~ stateName=\"$2\" ]]
  then
    echo "EVM did not respond with expected '$2', but with: $response"
    exit 1
  fi
}

function sendCmdToRUs
{
  response=$(sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 $1)
  if [[ ! $response =~ stateName=\"$2\" ]]
  then
    echo "RU1 did not respond with expected '$2', but with: $response"
    exit 1
  fi

  response=$(sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 $1)
  if [[ ! $response =~ stateName=\"$2\" ]]
  then
    echo "RU2 did not respond with expected '$2', but with: $response"
    exit 1
  fi
}

function sendCmdToBUs
{
  response=$(sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 $1)
  if [[ ! $response =~ stateName=\"$2\" ]]
  then
    echo "BU0 did not respond with expected '$2', but with: $response"
    exit 1
  fi

  response=$(sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 $1)
  if [[ ! $response =~ stateName=\"$2\" ]]
  then
    echo "BU1 did not respond with expected '$2', but with: $response"
    exit 1
  fi
}

function changeStates
{
  if [[ $1 == "Enable" ]]
  then
    runNumber=$(date "+%s")
    setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
    setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
    setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 runNumber unsignedInt $runNumber
    setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
    setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 runNumber unsignedInt $runNumber
  fi

  sendCmdToEVM $1 $2
  sendCmdToRUs $1 $2
  sleep 1
  sendCmdToBUs $1 $2
  sleep 1
}

function checkStates
{
  state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
  echo "RU0 state=$state"
  if [[ $state != $1 ]]
  then
    echo "Test failed"
    exit 1
  fi
  state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
  echo "RU1 state=$state"
  if [[ $state != $1 ]]
  then
    echo "Test failed"
    exit 1
  fi
  state=$(getParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 stateName xsd:string)
  echo "RU2 state=$state"
  if [[ $state != $1 ]]
  then
    echo "Test failed"
    exit 1
  fi
  state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
  echo "BU0 state=$state"
  if [[ $state != $1 ]]
  then
    echo "Test failed"
    exit 1
  fi
  state=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 stateName xsd:string)
  echo "BU1 state=$state"
  if [[ $state != $1 ]]
  then
    echo "Test failed"
    exit 1
  fi
}

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
sendCmdToLauncher RU2_SOAP_HOST_NAME RU2_LAUNCHER_PORT STARTXDAQRU2_SOAP_PORT
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
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME BU1_SOAP_PORT configure.cmd.xml

checkStates "Halted"

# Configure and enable pt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 3 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT pt::utcp::Application 4 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 3 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT pt::utcp::Application 4 Enable

changeStates Configure Configuring
sleep 1

checkStates "Ready"

changeStates Halt Halted

checkStates "Halted"

setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir/"

changeStates Configure Configuring
sleep 1

checkStates "Ready"

changeStates Enable Enabled

checkStates "Enabled"

echo "Building for 2 seconds"
sleep 2

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"

if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

checkStates "Enabled"

changeStates Halt Halted

checkStates "Halted"

changeStates Configure Configuring
sleep 1

checkStates "Ready"

changeStates Enable Enabled

checkStates "Enabled"

echo "Building for 2 seconds"
sleep 2

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"

if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

checkStates "Enabled"

echo "Stop EVM"
sendCmdToEVM Stop Draining
sleep 3

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "RU0 state=$state"
if [[ $state != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

# stop RUs and BUs
state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
echo "RU1 state=$state"
if [[ $state != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 stateName xsd:string)
echo "RU2 state=$state"
if [[ $state != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU0 state=$state"
if [[ $state != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 stateName xsd:string)
echo "BU1 state=$state"
if [[ $state != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

sendCmdToRUs Stop Draining
sendCmdToBUs Stop Draining
sleep 1

checkStates Ready

# restart run
changeStates Enable Enabled

checkStates "Enabled"

echo "Building for 2 seconds"
sleep 2

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"

if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

# fail the EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Fail

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "EVM state=$state"
if [[ "$state" != "Failed" ]]
then
  echo "Test failed"
  exit 1
fi

changeStates Halt Halted

checkStates "Halted"

changeStates Configure Configuring
sleep 1

checkStates "Ready"

changeStates Enable Enabled

checkStates "Enabled"

echo "Building for 2 seconds"
sleep 2

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"

if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

checkStates "Enabled"

# fail the RU
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Fail

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Failed" ]]
then
  echo "Test failed"
  exit 1
fi

changeStates Halt Halted

checkStates "Halted"

changeStates Configure Configuring
sleep 1

checkStates "Ready"

changeStates Enable Enabled

checkStates "Enabled"

echo "Building for 2 seconds"
sleep 2

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
nbEventsBuiltBU1=$(getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedLong)

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"

if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

if [[ $nbEventsBuiltBU1 -lt 1000 ]]
then
 echo "Test failed"
 exit 1
fi

checkStates "Enabled"

echo "Test launched successfully"
exit 0
