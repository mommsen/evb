#!/bin/sh
source ../setenv.sh

testCaseId=RUB_TESTER_HOME/data/$1

mkdir -p $testCaseId 

outputFileBU0=$testCaseId/server0.csv
outputFileBU1=$testCaseId/server1.csv
outputFileBU2=$testCaseId/server2.csv
outputFileBU3=$testCaseId/server3.csv
outputFileBU=$testCaseId/server.csv


#download BUs  e.g. /downloadMeasurments.sh dveb-b1b04-10-03 65442 Server 2 pippo.csv
./downloadMeasurements.sh BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 $outputFileBU0
./downloadMeasurements.sh BU1_SOAP_HOST_NAME BU1_SOAP_PORT gevb2g::BU 1 $outputFileBU1
./downloadMeasurements.sh BU2_SOAP_HOST_NAME BU2_SOAP_PORT gevb2g::BU 2 $outputFileBU2
./downloadMeasurements.sh BU3_SOAP_HOST_NAME BU3_SOAP_PORT gevb2g::BU 3 $outputFileBU3

echo "" >> $outputFileBU
cat $outputFileBU0 >> $outputFileBU
cat $outputFileBU1 >> $outputFileBU
cat $outputFileBU2 >> $outputFileBU
cat $outputFileBU3 >> $outputFileBU

