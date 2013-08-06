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
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
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

sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
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

#Set parameter
sleep 2
fragmentSize=$1
setParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 fedSize unsignedInt $fragmentSize
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 fedSize unsignedInt $fragmentSize
setParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 fedSize unsignedInt $fragmentSize
setParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 fedSize unsignedInt $fragmentSize
setParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 fedSize unsignedInt $fragmentSize
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 fedSize unsignedInt $fragmentSize
setParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 fedSize unsignedInt $fragmentSize
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 fedSize unsignedInt $fragmentSize
setParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 fedSize unsignedInt $fragmentSize
setParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 fedSize unsignedInt $fragmentSize
setParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 fedSize unsignedInt $fragmentSize
setParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 fedSize unsignedInt $fragmentSize

dummyFedPayloadSizeFEROL0=`getParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL1=`getParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL2=`getParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL3=`getParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL4=`getParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL5=`getParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL6=`getParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL7=`getParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL8=`getParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL9=`getParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL10=`getParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL11=`getParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 fedSize xsd:unsignedInt`

echo "DummyFEROL0 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL0"
echo "DummyFEROL1 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL1"
echo "DummyFEROL2 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL2"
echo "DummyFEROL3 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL3"
echo "DummyFEROL4 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL4"
echo "DummyFEROL5 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL5"
echo "DummyFEROL6 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL6"
echo "DummyFEROL7 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL7"
echo "DummyFEROL8 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL8"
echo "DummyFEROL9 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL9"
echo "DummyFEROL10 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL10"
echo "DummyFEROL11 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL11"

stdDev=$(echo $1 $2 | awk '{ print $1 * $2 }')
stdDev=$(printf "%0.f" $stdDev)
echo "Set fedSizeStdDev to $2"
setParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 fedSizeStdDev unsignedInt $stdDev
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 fedSizeStdDev unsignedInt $stdDev
setParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 fedSizeStdDev unsignedInt $stdDev
setParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 fedSizeStdDev unsignedInt $stdDev
setParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 fedSizeStdDev unsignedInt $stdDev
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 fedSizeStdDev unsignedInt $stdDev
setParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 fedSizeStdDev unsignedInt $stdDev
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 fedSizeStdDev unsignedInt $stdDev
setParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 fedSizeStdDev unsignedInt $stdDev
setParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 fedSizeStdDev unsignedInt $stdDev
setParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 fedSizeStdDev unsignedInt $stdDev
setParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 fedSizeStdDev unsignedInt $stdDev

dummyFedSizeStdDevFEROL0=`getParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL1=`getParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL2=`getParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL3=`getParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL4=`getParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL5=`getParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL6=`getParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL7=`getParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL8=`getParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL9=`getParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL10=`getParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL11=`getParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 fedSizeStdDev xsd:unsignedInt`
echo "DummyFEROL0 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL0"
echo "DummyFEROL1 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL1"
echo "DummyFEROL2 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL2"
echo "DummyFEROL3 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL3"
echo "DummyFEROL4 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL4"
echo "DummyFEROL5 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL5"
echo "DummyFEROL6 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL6"
echo "DummyFEROL7 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL7"
echo "DummyFEROL8 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL8"
echo "DummyFEROL9 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL9"
echo "DummyFEROL10 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL10"
echo "DummyFEROL11 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL11"

useLogNormal=$3
echo "Set useLogNormal to $useLogNormal"
setParam FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 useLogNormal boolean $useLogNormal
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 useLogNormal boolean $useLogNormal
setParam FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 useLogNormal boolean $useLogNormal
setParam FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 useLogNormal boolean $useLogNormal
setParam FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 useLogNormal boolean $useLogNormal
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 useLogNormal boolean $useLogNormal
setParam FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 useLogNormal boolean $useLogNormal
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 useLogNormal boolean $useLogNormal
setParam FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 useLogNormal boolean $useLogNormal
setParam FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 useLogNormal boolean $useLogNormal
setParam FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 useLogNormal boolean $useLogNormal
setParam FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 useLogNormal boolean $useLogNormal


#Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Configure
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 Configure
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 Configure
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 Configure
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 Configure
sendSimpleCmdToApp FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 Configure
sendSimpleCmdToApp FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 Configure
sendSimpleCmdToApp FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 Configure
sendSimpleCmdToApp FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 Configure

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure

sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Configure
sendSimpleCmdToApp BU2_SOAP_HOST_NAME BU2_SOAP_PORT evb::BU 2 Configure
sendSimpleCmdToApp BU3_SOAP_HOST_NAME BU3_SOAP_PORT evb::BU 3 Configure

sleep 1

#Enable EVM
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable
#Enable BUs
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable
sendSimpleCmdToApp BU1_SOAP_HOST_NAME BU1_SOAP_PORT evb::BU 1 Enable
sendSimpleCmdToApp BU2_SOAP_HOST_NAME BU2_SOAP_PORT evb::BU 2 Enable
sendSimpleCmdToApp BU3_SOAP_HOST_NAME BU3_SOAP_PORT evb::BU 3 Enable

#enable generator
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Enable
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 Enable
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 Enable
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 Enable
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 Enable
sendSimpleCmdToApp FEROL8_SOAP_HOST_NAME FEROL8_SOAP_PORT evb::test::DummyFEROL 8 Enable
sendSimpleCmdToApp FEROL9_SOAP_HOST_NAME FEROL9_SOAP_PORT evb::test::DummyFEROL 9 Enable
sendSimpleCmdToApp FEROL10_SOAP_HOST_NAME FEROL10_SOAP_PORT evb::test::DummyFEROL 10 Enable
sendSimpleCmdToApp FEROL11_SOAP_HOST_NAME FEROL11_SOAP_PORT evb::test::DummyFEROL 11 Enable

echo "Sending data for 10 seconds"
sleep 10

expectedEventSize=$((12*($fragmentSize+16))) #add FEROL header size

superFragmentSizeEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt`

echo "EVM superFragmentSize: $superFragmentSizeEVM"

if [[ $superFragmentSizeEVM -ne $expectedEventSize ]]
then
  echo "Test failed: expected $expectedEventSize"
  exit 1
fi

eventRateEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt`
echo "EVM eventRate: $eventRateEVM"

if [[ $eventRateEVM -lt 150000 ]]
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
