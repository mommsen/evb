#!/bin/sh

# Cleanup
testDir=/tmp/evb_test
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

echo "Sending data for 15 seconds"
sleep 15

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

echo "Wait for lumi section timeouts"
sleep 15

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

if [[ ! -s $testDir/BU-000/run$runNumber/EoLS.jsd ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoLS.jsd does not exist"
    exit 1
fi

if [[ ! -s $testDir/BU-000/run$runNumber/EoR.jsd ]]
then
    echo "Test failed: $testDir/BU-000/run$runNumber/EoR.jsd does not exist"
    exit 1
fi

runEventCounter=0;
runFileCounter=0;
runLsCounter=0;

for eolsFile in `ls $testDir/BU-000/run$runNumber/EoLS_*jsn`
do
    lumiSection=$(echo $eolsFile | sed -re 's/.*EoLS_([0-9]+).jsn/\1/')
    lsEventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)", "[0-9]+" \],/\1/p' $eolsFile)
    lsFileCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "([0-9]+)" \],/\1/p' $eolsFile)

    eventCounter=0
    fileCounter=0

    if [[ $lsFileCount -gt 0 ]]
    then
        ((runLsCounter++))

        for file in `ls $testDir/BU-000/run$runNumber/run${runNumber}_ls${lumiSection}_*raw`
        do
            ((fileCounter++))
            ((runFileCounter++))
            fileSize=$(stat -c%s $file)
            expectedEventCount=$(($fileSize/24576))

            jsonFile=$(echo $file | sed -re 's/.raw$/.jsn/')
            if [[ ! -s $jsonFile ]]
            then
                echo "Test failed: $jsonFile does not exist"
                exit 1
            fi

            eventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)" \],/\1/p' $jsonFile)
            if [[ $eventCount != $expectedEventCount ]]
            then
                echo "Test failed: expected $expectedEventCount, but found $eventCount events in JSON file"
                exit 1
            fi
            eventCounter=$(($eventCounter+$eventCount))
        done

    else

        fileCounter=$(ls $testDir/BU-000/run$runNumber/*[raw,jsn] | grep -c run${runNumber}_ls${lumiSection})

    fi

    if [[ $eventCounter != $lsEventCount ]]
    then
        echo "Test failed: expected $lsEventCount events in LS $lumiSection, but found $eventCounter events in raw data JSON files"
        exit 1
    fi
    if [[ $fileCounter != $lsFileCount ]]
    then
        echo "Test failed: expected $lsFileCount files for LS $lumiSection, but found $fileCounter raw data files"
        exit 1
    fi

    runEventCounter=$(($runEventCounter+$eventCounter))

done

eorFile="$testDir/BU-000/run$runNumber/EoR_$runNumber.jsn"

if [[ ! -s $eorFile ]]
then
    echo "Test failed: cannot locate the EoR json file $eorFile"
    exit 1
fi

runEventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)", "[0-9]+", "[0-9]+" \],/\1/p' $eorFile)
runFileCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "([0-9]+)", "[0-9]+" \],/\1/p' $eorFile)
runLsCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "[0-9]+", "([0-9]+)" \],/\1/p' $eorFile)

if [[ $runEventCounter != $runEventCount ]]
then
    echo "Test failed: expected $runEventCount events in run $runNumber, but found $runEventCounter events in raw data JSON files"
    exit 1
fi
if [[ $runFileCounter != $runFileCount ]]
then
    echo "Test failed: expected $runFileCount files for run $runNumber, but found $runFileCounter raw data files"
    exit 1
fi
if [[ $runLsCounter != $runLsCount ]]
then
    echo "Test failed: expected $runLsCount lumi sections for run $runNumber, but found raw data files from $runLsCounter lumi sections"
    exit 1
fi

echo "Test completed successfully"
exit 0