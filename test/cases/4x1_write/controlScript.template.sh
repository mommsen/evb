#!/bin/sh

source EVB_TESTER_HOME/cases/helpers.sh

# Cleanup
testDir=/tmp/evb_test
rm -rf $testDir
mkdir -p $testDir
echo "dummy HLT menu for EvB test" >> $testDir/HltConfig.py
echo "slc6_amd64_gcc472" >> $testDir/SCRAM_ARCH
echo "cmssw_noxdaq" >> $testDir/CMSSW_VERSION

# Emulate some left-over runs
dummyRuns="123456 234567 345678"
for run in $dummyRuns; do
    mkdir $testDir/run$run
done
touch $testDir/run234567/EoR_234567.jsn

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

# Configure and enable pt
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT pt::utcp::Application 3 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 4 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT pt::utcp::Application 3 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 4 Enable

# Set fragment size
#fragmentSize=15560
fragmentSize=1024
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 dummyFedSize unsignedInt $fragmentSize
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 dummyFedSize unsignedInt $fragmentSize
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 dummyFedSize unsignedInt $fragmentSize
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 dummyFedSize unsignedInt $fragmentSize

setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 rawDataDir string $testDir
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 metaDataDir string $testDir

# Configure all applications
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Configure
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure

sleep 1

runNumber=`date "+%s"`
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
setParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 runNumber unsignedInt $runNumber
setParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir"

#Enable EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Enable
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Enable

#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

echo "Sending data for 5 seconds"
sleep 5

superFragmentSize=$((16*$fragmentSize)) #add FEROL header size

superFragmentSizeEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt`
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne $superFragmentSize ]]
then
  echo "Test failed: expected $superFragmentSize"
  exit 1
fi

superFragmentSizeRU1=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt`
echo "RU1 superFragmentSize: $superFragmentSizeRU1"
if [[ $superFragmentSizeRU1 -ne $superFragmentSize ]]
then
  echo "Test failed: expected $superFragmentSize"
  exit 1
fi

superFragmentSizeRU2=`getParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 superFragmentSize xsd:unsignedInt`
echo "RU2 superFragmentSize: $superFragmentSizeRU2"
if [[ $superFragmentSizeRU2 -ne $superFragmentSize ]]
then
  echo "Test failed: expected $superFragmentSize"
  exit 1
fi

superFragmentSizeRU3=`getParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 superFragmentSize xsd:unsignedInt`
echo "RU3 superFragmentSize: $superFragmentSizeRU3"
if [[ $superFragmentSizeRU3 -ne $superFragmentSize ]]
then
  echo "Test failed: expected $superFragmentSize"
  exit 1
fi

eventRateEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt`
echo "EVM eventRate: $eventRateEVM"
if [[ $eventRateEVM -lt 100 ]]
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

eventSize=$(($fragmentSize*64))
eventSizeBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventSize xsd:unsignedInt`
echo "BU0 eventSize: $eventSizeBU0"
if [[ $eventSizeBU0 -ne $eventSize ]]
then
  echo "Test failed: expected $eventSize"
  exit 1
fi

eventRateBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventRate xsd:unsignedInt`
echo "BU0 eventRate: $eventRateBU0"
if [[ $eventRateBU0 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi

echo "BU0 bandwidth: $(($eventRateBU0*$eventSizeBU0))"

#Stop EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Stop
sleep 1

#Stop RUs
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Stop
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Stop
sendSimpleCmdToApp RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 Stop

sleep 2

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string`
echo "EVM state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string`
echo "RU0 state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 stateName xsd:string`
echo "RU1 state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

state=`getParam RU3_SOAP_HOST_NAME RU3_SOAP_PORT evb::RU 2 stateName xsd:string`
echo "RU2 state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

#Stop BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Stop
sleep 5

state=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string`
echo "BU state=$state"
if [[ "$state" != "Ready" ]]
then
  echo "Test failed"
  exit 1
fi

checkBuDir $testDir/run$runNumber 69632 1

for run in $dummyRuns; do
    if [[ ! -e $testDir/run$run/EoR_$run.jsn ]]
    then
        echo "Test failed: creation of dummy EoR for run $run failed"
        exit 1
    fi
done


echo "Test completed successfully"
exit 0
