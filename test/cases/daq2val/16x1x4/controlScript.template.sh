#!/bin/sh

# Launch executive processes
sendCmdToLauncher FEROL0_SOAP_HOST_NAME FEROL0_LAUNCHER_PORT STARTXDAQFEROL0_SOAP_PORT
sendCmdToLauncher FEROL1_SOAP_HOST_NAME FEROL1_LAUNCHER_PORT STARTXDAQFEROL1_SOAP_PORT
sendCmdToLauncher FEROL2_SOAP_HOST_NAME FEROL2_LAUNCHER_PORT STARTXDAQFEROL2_SOAP_PORT
sendCmdToLauncher FEROL3_SOAP_HOST_NAME FEROL3_LAUNCHER_PORT STARTXDAQFEROL3_SOAP_PORT
sendCmdToLauncher FEROL4_SOAP_HOST_NAME FEROL4_LAUNCHER_PORT STARTXDAQFEROL4_SOAP_PORT
sendCmdToLauncher FEROL5_SOAP_HOST_NAME FEROL5_LAUNCHER_PORT STARTXDAQFEROL5_SOAP_PORT
sendCmdToLauncher FEROL6_SOAP_HOST_NAME FEROL6_LAUNCHER_PORT STARTXDAQFEROL6_SOAP_PORT
sendCmdToLauncher FEROL7_SOAP_HOST_NAME FEROL7_LAUNCHER_PORT STARTXDAQFEROL7_SOAP_PORT
sendCmdToLauncher FEROL8_SOAP_HOST_NAME FEROL8_LAUNCHER_PORT STARTXDAQFEROL8_SOAP_PORT
sendCmdToLauncher FEROL9_SOAP_HOST_NAME FEROL9_LAUNCHER_PORT STARTXDAQFEROL9_SOAP_PORT
sendCmdToLauncher FEROL10_SOAP_HOST_NAME FEROL10_LAUNCHER_PORT STARTXDAQFEROL10_SOAP_PORT
sendCmdToLauncher FEROL11_SOAP_HOST_NAME FEROL11_LAUNCHER_PORT STARTXDAQFEROL11_SOAP_PORT
sendCmdToLauncher FEROL12_SOAP_HOST_NAME FEROL12_LAUNCHER_PORT STARTXDAQFEROL12_SOAP_PORT
sendCmdToLauncher FEROL13_SOAP_HOST_NAME FEROL13_LAUNCHER_PORT STARTXDAQFEROL13_SOAP_PORT
sendCmdToLauncher FEROL14_SOAP_HOST_NAME FEROL14_LAUNCHER_PORT STARTXDAQFEROL14_SOAP_PORT
sendCmdToLauncher FEROL15_SOAP_HOST_NAME FEROL15_LAUNCHER_PORT STARTXDAQFEROL15_SOAP_PORT
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT
sendCmdToLauncher BU1_SOAP_HOST_NAME BU1_LAUNCHER_PORT STARTXDAQBU1_SOAP_PORT
sendCmdToLauncher BU2_SOAP_HOST_NAME BU2_LAUNCHER_PORT STARTXDAQBU2_SOAP_PORT
sendCmdToLauncher BU3_SOAP_HOST_NAME BU3_LAUNCHER_PORT STARTXDAQBU3_SOAP_PORT

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
if ! webPingXDAQ FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT 5
then
    echo "Test failed"
    exit 1
fi
if ! webPingXDAQ FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT 5
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
if ! webPingXDAQ BU1_SOAP_HOST_NAME BU1_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU2_SOAP_HOST_NAME BU2_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ BU3_SOAP_HOST_NAME BU3_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi

# Configure all executives
sendCmdToExecutive FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT configure.cmd.xml

sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU1_SOAP_HOST_NAME BU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU2_SOAP_HOST_NAME BU2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU3_SOAP_HOST_NAME BU3_SOAP_PORT configure.cmd.xml

# Configure and enable ptutcp
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Configure
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::frl::Application 4 Configure
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::frl::Application 5 Configure
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::frl::Application 6 Configure
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::frl::Application 7 Configure
sendSimpleCmdToApp FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT pt::frl::Application 8 Configure
sendSimpleCmdToApp FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT pt::frl::Application 9 Configure
sendSimpleCmdToApp FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT pt::frl::Application 10 Configure
sendSimpleCmdToApp FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT pt::frl::Application 11 Configure
sendSimpleCmdToApp FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT pt::frl::Application 12 Configure
sendSimpleCmdToApp FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT pt::frl::Application 13 Configure
sendSimpleCmdToApp FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT pt::frl::Application 14 Configure
sendSimpleCmdToApp FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT pt::frl::Application 15 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Enable
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::frl::Application 4 Enable
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::frl::Application 5 Enable
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::frl::Application 6 Enable
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::frl::Application 7 Enable
sendSimpleCmdToApp FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT pt::frl::Application 8 Enable
sendSimpleCmdToApp FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT pt::frl::Application 9 Enable
sendSimpleCmdToApp FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT pt::frl::Application 10 Enable
sendSimpleCmdToApp FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT pt::frl::Application 11 Enable
sendSimpleCmdToApp FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT pt::frl::Application 12 Enable
sendSimpleCmdToApp FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT pt::frl::Application 13 Enable
sendSimpleCmdToApp FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT pt::frl::Application 14 Enable
sendSimpleCmdToApp FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT pt::frl::Application 15 Enable

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
setParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT Client 8 currentSize unsignedLong $fragmentSize
setParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT Client 9 currentSize unsignedLong $fragmentSize
setParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT Client 10 currentSize unsignedLong $fragmentSize
setParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT Client 11 currentSize unsignedLong $fragmentSize
setParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT Client 12 currentSize unsignedLong $fragmentSize
setParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT Client 13 currentSize unsignedLong $fragmentSize
setParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT Client 14 currentSize unsignedLong $fragmentSize
setParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT Client 15 currentSize unsignedLong $fragmentSize

dummyFedPayloadSizeFEROL0=`getParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL1=`getParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL2=`getParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL3=`getParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL4=`getParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL5=`getParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL6=`getParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL7=`getParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL8=`getParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT Client 8 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL9=`getParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT Client 9 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL10=`getParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT Client 10 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL11=`getParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT Client 11 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL12=`getParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT Client 12 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL13=`getParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT Client 13 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL14=`getParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT Client 14 currentSize  xsd:unsignedLong`
dummyFedPayloadSizeFEROL15=`getParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT Client 15 currentSize  xsd:unsignedLong`

echo "Client0 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL0"
echo "Client1 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL1"
echo "Client2 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL2"
echo "Client3 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL3"
echo "Client4 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL4"
echo "Client5 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL5"
echo "Client6 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL6"
echo "Client7 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL7"
echo "Client8 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL8"
echo "Client9 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL9"
echo "Client10 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL10"
echo "Client11 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL11"
echo "Client12 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL12"
echo "Client13 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL13"
echo "Client14 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL14"
echo "Client15 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL15"

#Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 Configure

sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Configure
sendSimpleCmdToApp BU2_SOAP_HOST_NAME BU2_SOAP_PORT evb::BU 2 Configure
sendSimpleCmdToApp BU3_SOAP_HOST_NAME BU3_SOAP_PORT evb::BU 3 Configure

sleep 1

#Enable EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable
#Enable RU
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 Enable
#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Enable
sendSimpleCmdToApp BU2_SOAP_HOST_NAME BU2_SOAP_PORT evb::BU 2 Enable
sendSimpleCmdToApp BU3_SOAP_HOST_NAME BU3_SOAP_PORT evb::BU 3 Enable

#enebale generator
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 start
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 start
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 start
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 start
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 start
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 start
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 start
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 start
sendSimpleCmdToApp FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT Client 8 start
sendSimpleCmdToApp FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT Client 9 start
sendSimpleCmdToApp FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT Client 10 start
sendSimpleCmdToApp FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT Client 11 start
sendSimpleCmdToApp FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT Client 12 start
sendSimpleCmdToApp FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT Client 13 start
sendSimpleCmdToApp FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT Client 14 start
sendSimpleCmdToApp FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT Client 15 start

echo "Sending data for 10 seconds"
sleep 10

nbCounterServerFEROL0=`getParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT Client 0 counter  xsd:unsignedLong`
nbCounterServerFEROL1=`getParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT Client 1 counter  xsd:unsignedLong`
nbCounterServerFEROL2=`getParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT Client 2 counter  xsd:unsignedLong`
nbCounterServerFEROL3=`getParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT Client 3 counter  xsd:unsignedLong`
nbCounterServerFEROL4=`getParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT Client 4 counter  xsd:unsignedLong`
nbCounterServerFEROL5=`getParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT Client 5 counter  xsd:unsignedLong`
nbCounterServerFEROL6=`getParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT Client 6 counter  xsd:unsignedLong`
nbCounterServerFEROL7=`getParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT Client 7 counter  xsd:unsignedLong`
nbCounterServerFEROL8=`getParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT Client 8 counter  xsd:unsignedLong`
nbCounterServerFEROL9=`getParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT Client 9 counter  xsd:unsignedLong`
nbCounterServerFEROL10=`getParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT Client 10 counter  xsd:unsignedLong`
nbCounterServerFEROL11=`getParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT Client 11 counter  xsd:unsignedLong`
nbCounterServerFEROL12=`getParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT Client 12 counter  xsd:unsignedLong`
nbCounterServerFEROL13=`getParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT Client 13 counter  xsd:unsignedLong`
nbCounterServerFEROL14=`getParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT Client 14 counter  xsd:unsignedLong`
nbCounterServerFEROL15=`getParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT Client 15 counter  xsd:unsignedLong`

echo "Client 0 nbEvtsBuilt: $nbCounterServerFEROL0"
echo "Client 1 nbEvtsBuilt: $nbCounterServerFEROL1"
echo "Client 2 nbEvtsBuilt: $nbCounterServerFEROL2"
echo "Client 3 nbEvtsBuilt: $nbCounterServerFEROL3"
echo "Client 4 nbEvtsBuilt: $nbCounterServerFEROL4"
echo "Client 5 nbEvtsBuilt: $nbCounterServerFEROL5"
echo "Client 6 nbEvtsBuilt: $nbCounterServerFEROL6"
echo "Client 7 nbEvtsBuilt: $nbCounterServerFEROL7"
echo "Client 8 nbEvtsBuilt: $nbCounterServerFEROL8"
echo "Client 9 nbEvtsBuilt: $nbCounterServerFEROL9"
echo "Client 10 nbEvtsBuilt: $nbCounterServerFEROL10"
echo "Client 11 nbEvtsBuilt: $nbCounterServerFEROL11"
echo "Client 12 nbEvtsBuilt: $nbCounterServerFEROL12"
echo "Client 13 nbEvtsBuilt: $nbCounterServerFEROL13"
echo "Client 14 nbEvtsBuilt: $nbCounterServerFEROL14"
echo "Client 15 nbEvtsBuilt: $nbCounterServerFEROL15"

if [[ $nbCounterServerFEROL0 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL1 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL2 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL3 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL4 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL5 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL6 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL7 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL8 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL9 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL10 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL11 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL12 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL13 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL14 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbCounterServerFEROL15 -lt 100 ]]
then
  echo "Test failed"
  exit 1
fi


nbEventsBuiltBU0=`getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedInt`
nbEventsBuiltBU1=`getParam BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 nbEventsBuilt xsd:unsignedInt`
nbEventsBuiltBU2=`getParam BU2_SOAP_HOST_NAME BU2_SOAP_PORT evb::BU 2 nbEventsBuilt xsd:unsignedInt`
nbEventsBuiltBU3=`getParam BU3_SOAP_HOST_NAME BU3_SOAP_PORT evb::BU 3 nbEventsBuilt xsd:unsignedInt`

echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
echo "BU1 nbEventsBuilt: $nbEventsBuiltBU1"
echo "BU2 nbEventsBuilt: $nbEventsBuiltBU2"
echo "BU3 nbEventsBuilt: $nbEventsBuiltBU3"

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
if [[ $nbEventsBuiltBU2 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi
if [[ $nbEventsBuiltBU3 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

echo "Test launched successfully"
exit 0
