# Helper function for test case scripts

function checkBuDir {
    runDir=$1
    eventSize=$2
    singleBU=$3

    ls --full-time -rt $runDir

    if [[ -x $runDir/open ]]
    then
        echo "Test failed: $runDir/open exists"
        exit 1
    fi

    for jsdFile in rawData.jsd EoLS.jsd EoR.jsd ; do
        if [[ ! -s $runDir/jsd/${jsdFile} ]]
        then
            echo "Test failed: $runDir/jsd/${jsdFile} does not exist"
            exit 1
        fi
    done

    for hltFile in HltConfig.py CMSSW_VERSION SCRAM_ARCH ; do
        if [[ ! -s $runDir/hlt/${hltFile} ]]
        then
            echo "Test failed: $runDir/hlt/${hltFile} does not exist"
            exit 1
        fi

        if [[ "x$(diff -q $testDir/${hltFile} $runDir/hlt/${hltFile})" != "x" ]]
        then
            echo "Test failed: $testDir/${hltFile} $runDir/hlt/${hltFile} differ:"
            diff $testDir/${hltFile} $runDir/hlt/${hltFile}
            exit 1
        fi
    done

    runEventCounter=0;
    runFileCounter=0;
    runLsCounter=0;
    lastLumiSection=0;

    for eolsFile in $(ls $runDir/*_EoLS.jsn)
    do
        lumiSection=$(echo $eolsFile | sed -re 's/.*_ls([0-9]+)_EoLS.jsn/\1/')
        lsEventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)", "[0-9]+", "[0-9]+" \],/\1/p' $eolsFile)
        lsFileCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "([0-9]+)", "[0-9]+" \],/\1/p' $eolsFile)
        totalEventCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "[0-9]+", "([0-9]+)" \],/\1/p' $eolsFile)

        if [[ $singleBU == 1 && $lsEventCount != $totalEventCount ]]
        then
            echo "Test failed: total event count $totalEventCount does not match $lsEventCount in $eolsFile"
            exit 1
        fi

        eventCounter=0
        fileCounter=0

        if [[ $lsFileCount -gt 0 ]]
        then
            ((runLsCounter++))
            lastLumiSection=$(echo $lumiSection | sed -re 's/0*([0-9]+)$/\1/')

            for jsonFile in `ls $runDir/run${runNumber}_ls${lumiSection}_index*jsn`
            do
                ((fileCounter++))
                ((runFileCounter++))
                eventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)" \],/\1/p' $jsonFile)
                eventCounter=$(($eventCounter+$eventCount))

                if [[ $eventSize -gt 0 ]]
                then
                    rawFile=$(echo $jsonFile | sed -re 's/.jsn$/.raw/')
                    fileSize=$(stat -c%s $rawFile)
                    expectedEventCount=$(($fileSize/$eventSize))

                    if [[ ! -s $rawFile ]]
                    then
                        echo "Test failed: $rawFile does not exist"
                        exit 1
                    fi

                    if [[ $eventCount != $expectedEventCount ]]
                    then
                        echo "Test failed: expected $expectedEventCount, but found $eventCount events in JSON file $jsonFile"
                        exit 1
                    fi
                fi
            done

        else

            fileCounter=$(ls $runDir/*[raw,jsn] | grep -v EoLS | grep -c run${runNumber}_ls${lumiSection})

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

    eorFile="$runDir/run${runNumber}_ls0000_EoR.jsn"

    if [[ ! -s $eorFile ]]
    then
        echo "Test failed: cannot locate the EoR json file $eorFile"
        exit 1
    fi

    runEventCount=$(sed -nre 's/   "data" : \[ "([0-9]+)", "[0-9]+", "[0-9]+", "[0-9]+" \],/\1/p' $eorFile)
    runFileCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "([0-9]+)", "[0-9]+", "[0-9]+" \],/\1/p' $eorFile)
    runLsCount=$(sed -nre 's/   "data" : \[ "[0-9]+", "[0-9]+", "([0-9]+)", "[0-9]+" \],/\1/p' $eorFile)
    runLastLumiSection=$(sed -nre 's/   "data" : \[ "[0-9]+", "[0-9]+", "[0-9]+", "([0-9]+)" \],/\1/p' $eorFile)

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
    if [[ $runLastLumiSection != $lastLumiSection ]]
    then
        echo "Test failed: expected last LS $runLastLumiSection for run $runNumber, but found raw data files up to lumi section $lastLumiSection"
        exit 1
    fi
}
