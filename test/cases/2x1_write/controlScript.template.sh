#!/bin/sh

# Cleanup
testDir=/tmp/evb_test
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

runNumber=`date "+%s"`

#Enable EVM
setParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable

#Enable RU
setParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 runNumber unsignedInt $runNumber
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable

#Enable BUs
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 runNumber unsignedInt $runNumber
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 hltParameterSetURL string "file://$testDir/"
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

echo "Sending data for 15 seconds"
sleep 15

superFragmentSizeEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt`
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne 2048 ]]
then
  echo "Test failed: expected 2048"
  exit 1
fi

superFragmentSizeRU1=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt`
echo "RU1 superFragmentSize: $superFragmentSizeRU1"
if [[ $superFragmentSizeRU1 -ne 16384 ]]
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
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Stop

sleep 2

state=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string`
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

state=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string`
echo "RU state=$state"
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

ls --full-time -rt $testDir/run$runNumber

if [[ -x $testDir/run$runNumber/open ]]
then
    echo "Test failed: $testDir/run$runNumber/open exists"
    exit 1
fi

for jsdFile in rawData.jsd EoLS.jsd EoR.jsd ; do
    if [[ ! -s $testDir/run$runNumber/jsd/${jsdFile} ]]
    then
        echo "Test failed: $testDir/run$runNumber/jsd/${jsdFile} does not exist"
        exit 1
    fi
done

for hltFile in HltConfig.py CMSSW_VERSION SCRAM_ARCH ; do
    if [[ ! -s $testDir/run$runNumber/hlt/${hltFile} ]]
    then
        echo "Test failed: $testDir/run$runNumber/hlt/${hltFile} does not exist"
        exit 1
    fi

    if [[ "x`diff -q $testDir/${hltFile} $testDir/run$runNumber/hlt/${hltFile}`" != "x" ]]
    then
        echo "Test failed: $testDir/${hltFile} $testDir/run$runNumber/hlt/${hltFile} differ:"
        diff $testDir/${hltFile} $testDir/run$runNumber/hlt/${hltFile}
        exit 1
    fi
done

runEventCounter=0;
runFileCounter=0;
runLsCounter=0;

for eolsFile in `ls $testDir/run$runNumber/EoLS_*jsn`
do
    lumiSection=$(echo $eolsFile | sed -re 's/.*EoLS_([0-9]+).jsn/\1/')
    lsEventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)", "[0-9]+", "[0-9]+" \],/\1/p' $eolsFile)
    lsFileCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "([0-9]+)", "[0-9]+" \],/\1/p' $eolsFile)
    totalEventCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "[0-9]+", "([0-9]+)" \],/\1/p' $eolsFile)

    if [[ $lsEventCount != $totalEventCount ]]
    then
        echo "Test failed: total event count $totalEventCount does not match $lsEventCount in $eolsFile"
        exit 1
    fi

    eventCounter=0
    fileCounter=0

    if [[ $lsFileCount -gt 0 ]]
    then
        ((runLsCounter++))

        for file in `ls $testDir/run$runNumber/run${runNumber}_ls${lumiSection}_*raw`
        do
            ((fileCounter++))
            ((runFileCounter++))
            fileSize=$(stat -c%s $file)
            expectedEventCount=$(($fileSize/20480))

            jsonFile=$(echo $file | sed -re 's/.raw$/.jsn/')
            if [[ ! -s $jsonFile ]]
            then
                echo "Test failed: $jsonFile does not exist"
                exit 1
            fi

            eventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)" \],/\1/p' $jsonFile)
            if [[ $eventCount != $expectedEventCount ]]
            then
                echo "Test failed: expected $expectedEventCount, but found $eventCount events in JSON file $jsonFile"
                exit 1
            fi
            eventCounter=$(($eventCounter+$eventCount))
        done

    else

        fileCounter=$(ls $testDir/run$runNumber/*[raw,jsn] | grep -c run${runNumber}_ls${lumiSection})

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

eorFile="$testDir/run$runNumber/EoR_$runNumber.jsn"

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
