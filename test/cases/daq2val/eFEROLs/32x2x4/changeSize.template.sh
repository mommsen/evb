#!/bin/sh
source ../setenv.sh
#Halt Client and Server
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::InputEmulator 0 Halt


sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT gevb2g::EVM 0 Halt

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::RU 0 Halt

sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 Halt

#Set parameter
sleep 2
fragmentSize=2048
setParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 currentSize unsignedLong $fragmentSize
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 currentSize unsignedLong $fragmentSize
setParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 currentSize unsignedLong $fragmentSize
setParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 currentSize unsignedLong $fragmentSize
setParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 currentSize unsignedLong $fragmentSize
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 currentSize unsignedLong $fragmentSize
setParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 currentSize unsignedLong $fragmentSize
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 currentSize unsignedLong $fragmentSize
setParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 currentSize unsignedLong 16384
setParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT gevb2g::BU 1 currentSize unsignedLong 16384 
setParam BU2_SOAP_HOST_NAME BU2_SOAP_PORT gevb2g::BU 2 currentSize unsignedLong 16384
setParam BU3_SOAP_HOST_NAME BU3_SOAP_PORT gevb2g::BU 3 currentSize unsignedLong 16384 

dummyFedPayloadSizeFEROL0=`getParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL1=`getParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL2=`getParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL3=`getParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL4=`getParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL5=`getParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL6=`getParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL7=`getParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT gevb2g::BU 1 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeBU2=`getParam BU2_SOAP_HOST_NAME BU2_SOAP_PORT gevb2g::BU 2 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeBU3=`getParam BU3_SOAP_HOST_NAME BU3_SOAP_PORT gevb2g::BU 3 currentSize  xsd:unsignedLong`
echo "Client0 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL0"
echo "Client1 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL1"
echo "Client2 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL2"
echo "Client3 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL3"
echo "Client4 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL4"
echo "Client5 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL5"
echo "Client6 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL6"
echo "Client7 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL7"
echo "Client8 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL8"
echo "Server0 dummyFedPayloadSize: $dummyFedPayloadSizeBU0"
echo "Server1 dummyFedPayloadSize: $dummyFedPayloadSizeBU1"
echo "Server2 dummyFedPayloadSize: $dummyFedPayloadSizeBU2"
echo "Server3 dummyFedPayloadSize: $dummyFedPayloadSizeBU3"


#Configure

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::InputEmulator 0 Configure

sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT gevb2g::EVM 0 Configure

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::RU 0 Configure

sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 Configure




dummyFedPayloadSizeRU0=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::InputEmulator 0 Mean  xsd:unsignedLong`
dummyFedPayloadSizeBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 currentSize  xsd:unsignedLong`
echo "Client0 dummyFedPayloadSize: $dummyFedPayloadSizeRU0"
echo "Server0 dummyFedPayloadSize: $dummyFedPayloadSizeBU0"

#Enable RUs
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::RU 0 Enable

#Enable EVMs
sendSimpleCmdToApp EVM0_SOAP_HOST_NAME EVM0_SOAP_PORT gevb2g::EVM 0 Enable
#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT gevb2g::BU 0 Enable


#Enable Input
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT gevb2g::InputEmulator 0 Enable
