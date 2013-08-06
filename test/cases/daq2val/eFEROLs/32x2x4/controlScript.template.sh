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
sendCmdToLauncher FEROL16_SOAP_HOST_NAME FEROL16_LAUNCHER_PORT STARTXDAQFEROL16_SOAP_PORT
sendCmdToLauncher FEROL17_SOAP_HOST_NAME FEROL17_LAUNCHER_PORT STARTXDAQFEROL17_SOAP_PORT
sendCmdToLauncher FEROL18_SOAP_HOST_NAME FEROL18_LAUNCHER_PORT STARTXDAQFEROL18_SOAP_PORT
sendCmdToLauncher FEROL19_SOAP_HOST_NAME FEROL19_LAUNCHER_PORT STARTXDAQFEROL19_SOAP_PORT
sendCmdToLauncher FEROL20_SOAP_HOST_NAME FEROL20_LAUNCHER_PORT STARTXDAQFEROL20_SOAP_PORT
sendCmdToLauncher FEROL21_SOAP_HOST_NAME FEROL21_LAUNCHER_PORT STARTXDAQFEROL21_SOAP_PORT
sendCmdToLauncher FEROL22_SOAP_HOST_NAME FEROL22_LAUNCHER_PORT STARTXDAQFEROL22_SOAP_PORT
sendCmdToLauncher FEROL23_SOAP_HOST_NAME FEROL23_LAUNCHER_PORT STARTXDAQFEROL23_SOAP_PORT
sendCmdToLauncher FEROL24_SOAP_HOST_NAME FEROL24_LAUNCHER_PORT STARTXDAQFEROL24_SOAP_PORT
sendCmdToLauncher FEROL25_SOAP_HOST_NAME FEROL25_LAUNCHER_PORT STARTXDAQFEROL25_SOAP_PORT
sendCmdToLauncher FEROL26_SOAP_HOST_NAME FEROL26_LAUNCHER_PORT STARTXDAQFEROL26_SOAP_PORT
sendCmdToLauncher FEROL27_SOAP_HOST_NAME FEROL27_LAUNCHER_PORT STARTXDAQFEROL27_SOAP_PORT
sendCmdToLauncher FEROL28_SOAP_HOST_NAME FEROL28_LAUNCHER_PORT STARTXDAQFEROL28_SOAP_PORT
sendCmdToLauncher FEROL29_SOAP_HOST_NAME FEROL29_LAUNCHER_PORT STARTXDAQFEROL29_SOAP_PORT
sendCmdToLauncher FEROL30_SOAP_HOST_NAME FEROL30_LAUNCHER_PORT STARTXDAQFEROL30_SOAP_PORT
sendCmdToLauncher FEROL31_SOAP_HOST_NAME FEROL31_LAUNCHER_PORT STARTXDAQFEROL31_SOAP_PORT
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
if ! webPingXDAQ FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT 5
then
  echo "Test failed"
  exit 1
fi
if ! webPingXDAQ FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT 5
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
sendCmdToExecutive FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT configure.cmd.xml

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
sendSimpleCmdToApp FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT pt::frl::Application 16 Configure
sendSimpleCmdToApp FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT pt::frl::Application 17 Configure
sendSimpleCmdToApp FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT pt::frl::Application 18 Configure
sendSimpleCmdToApp FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT pt::frl::Application 19 Configure
sendSimpleCmdToApp FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT pt::frl::Application 20 Configure
sendSimpleCmdToApp FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT pt::frl::Application 21 Configure
sendSimpleCmdToApp FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT pt::frl::Application 22 Configure
sendSimpleCmdToApp FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT pt::frl::Application 23 Configure
sendSimpleCmdToApp FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT pt::frl::Application 24 Configure
sendSimpleCmdToApp FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT pt::frl::Application 25 Configure
sendSimpleCmdToApp FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT pt::frl::Application 26 Configure
sendSimpleCmdToApp FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT pt::frl::Application 27 Configure
sendSimpleCmdToApp FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT pt::frl::Application 28 Configure
sendSimpleCmdToApp FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT pt::frl::Application 29 Configure
sendSimpleCmdToApp FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT pt::frl::Application 30 Configure
sendSimpleCmdToApp FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT pt::frl::Application 31 Configure
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
sendSimpleCmdToApp FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT pt::frl::Application 16 Enable
sendSimpleCmdToApp FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT pt::frl::Application 17 Enable
sendSimpleCmdToApp FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT pt::frl::Application 18 Enable
sendSimpleCmdToApp FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT pt::frl::Application 19 Enable
sendSimpleCmdToApp FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT pt::frl::Application 20 Enable
sendSimpleCmdToApp FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT pt::frl::Application 21 Enable
sendSimpleCmdToApp FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT pt::frl::Application 22 Enable
sendSimpleCmdToApp FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT pt::frl::Application 23 Enable
sendSimpleCmdToApp FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT pt::frl::Application 24 Enable
sendSimpleCmdToApp FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT pt::frl::Application 25 Enable
sendSimpleCmdToApp FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT pt::frl::Application 26 Enable
sendSimpleCmdToApp FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT pt::frl::Application 27 Enable
sendSimpleCmdToApp FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT pt::frl::Application 28 Enable
sendSimpleCmdToApp FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT pt::frl::Application 29 Enable
sendSimpleCmdToApp FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT pt::frl::Application 30 Enable
sendSimpleCmdToApp FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT pt::frl::Application 31 Enable

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
setParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 fedSize unsignedInt $fragmentSize
setParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 fedSize unsignedInt $fragmentSize
setParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 fedSize unsignedInt $fragmentSize
setParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 fedSize unsignedInt $fragmentSize
setParam FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 fedSize unsignedInt $fragmentSize
setParam FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 fedSize unsignedInt $fragmentSize
setParam FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 fedSize unsignedInt $fragmentSize
setParam FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 fedSize unsignedInt $fragmentSize
setParam FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 fedSize unsignedInt $fragmentSize
setParam FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 fedSize unsignedInt $fragmentSize
setParam FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 fedSize unsignedInt $fragmentSize
setParam FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 fedSize unsignedInt $fragmentSize
setParam FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 fedSize unsignedInt $fragmentSize
setParam FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 fedSize unsignedInt $fragmentSize
setParam FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 fedSize unsignedInt $fragmentSize
setParam FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 fedSize unsignedInt $fragmentSize
setParam FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 fedSize unsignedInt $fragmentSize
setParam FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 fedSize unsignedInt $fragmentSize
setParam FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 fedSize unsignedInt $fragmentSize
setParam FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 fedSize unsignedInt $fragmentSize

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
dummyFedPayloadSizeFEROL12=`getParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL13=`getParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL14=`getParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL15=`getParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL16=`getParam FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL17=`getParam FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL18=`getParam FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL19=`getParam FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL20=`getParam FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL21=`getParam FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL22=`getParam FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL23=`getParam FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL24=`getParam FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL25=`getParam FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL26=`getParam FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL27=`getParam FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL28=`getParam FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL29=`getParam FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL30=`getParam FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 fedSize xsd:unsignedInt`
dummyFedPayloadSizeFEROL31=`getParam FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 fedSize xsd:unsignedInt`

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
echo "DummyFEROL12 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL12"
echo "DummyFEROL13 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL13"
echo "DummyFEROL14 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL14"
echo "DummyFEROL15 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL15"
echo "DummyFEROL16 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL16"
echo "DummyFEROL17 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL17"
echo "DummyFEROL18 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL18"
echo "DummyFEROL19 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL19"
echo "DummyFEROL20 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL20"
echo "DummyFEROL21 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL21"
echo "DummyFEROL22 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL22"
echo "DummyFEROL23 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL23"
echo "DummyFEROL24 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL24"
echo "DummyFEROL25 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL25"
echo "DummyFEROL26 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL26"
echo "DummyFEROL27 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL27"
echo "DummyFEROL28 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL28"
echo "DummyFEROL29 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL29"
echo "DummyFEROL30 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL30"
echo "DummyFEROL31 dummyFedPayloadSize: $dummyFedPayloadSizeFEROL31"

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
setParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 fedSizeStdDev unsignedInt $stdDev
setParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 fedSizeStdDev unsignedInt $stdDev
setParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 fedSizeStdDev unsignedInt $stdDev
setParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 fedSizeStdDev unsignedInt $stdDev
setParam FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 fedSizeStdDev unsignedInt $stdDev
setParam FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 fedSizeStdDev unsignedInt $stdDev
setParam FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 fedSizeStdDev unsignedInt $stdDev
setParam FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 fedSizeStdDev unsignedInt $stdDev
setParam FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 fedSizeStdDev unsignedInt $stdDev
setParam FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 fedSizeStdDev unsignedInt $stdDev
setParam FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 fedSizeStdDev unsignedInt $stdDev
setParam FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 fedSizeStdDev unsignedInt $stdDev
setParam FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 fedSizeStdDev unsignedInt $stdDev
setParam FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 fedSizeStdDev unsignedInt $stdDev
setParam FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 fedSizeStdDev unsignedInt $stdDev
setParam FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 fedSizeStdDev unsignedInt $stdDev
setParam FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 fedSizeStdDev unsignedInt $stdDev
setParam FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 fedSizeStdDev unsignedInt $stdDev
setParam FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 fedSizeStdDev unsignedInt $stdDev
setParam FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 fedSizeStdDev unsignedInt $stdDev

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
dummyFedSizeStdDevFEROL12=`getParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL13=`getParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL14=`getParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL15=`getParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL16=`getParam FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL17=`getParam FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL18=`getParam FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL19=`getParam FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL20=`getParam FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL21=`getParam FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL22=`getParam FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL23=`getParam FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL24=`getParam FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL25=`getParam FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL26=`getParam FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL27=`getParam FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL28=`getParam FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL29=`getParam FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL30=`getParam FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 fedSizeStdDev xsd:unsignedInt`
dummyFedSizeStdDevFEROL31=`getParam FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 fedSizeStdDev xsd:unsignedInt`
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
echo "DummyFEROL12 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL12"
echo "DummyFEROL13 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL13"
echo "DummyFEROL14 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL14"
echo "DummyFEROL15 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL15"
echo "DummyFEROL16 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL16"
echo "DummyFEROL17 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL17"
echo "DummyFEROL18 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL18"
echo "DummyFEROL19 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL19"
echo "DummyFEROL20 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL20"
echo "DummyFEROL21 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL21"
echo "DummyFEROL22 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL22"
echo "DummyFEROL23 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL23"
echo "DummyFEROL24 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL24"
echo "DummyFEROL25 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL25"
echo "DummyFEROL26 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL26"
echo "DummyFEROL27 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL27"
echo "DummyFEROL28 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL28"
echo "DummyFEROL29 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL29"
echo "DummyFEROL30 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL30"
echo "DummyFEROL31 dummyFedSizeStdDev: $dummyFedSizeStdDevFEROL31"

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
setParam FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 useLogNormal boolean $useLogNormal
setParam FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 useLogNormal boolean $useLogNormal
setParam FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 useLogNormal boolean $useLogNormal
setParam FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 useLogNormal boolean $useLogNormal
setParam FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 useLogNormal boolean $useLogNormal
setParam FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 useLogNormal boolean $useLogNormal
setParam FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 useLogNormal boolean $useLogNormal
setParam FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 useLogNormal boolean $useLogNormal
setParam FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 useLogNormal boolean $useLogNormal
setParam FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 useLogNormal boolean $useLogNormal
setParam FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 useLogNormal boolean $useLogNormal
setParam FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 useLogNormal boolean $useLogNormal
setParam FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 useLogNormal boolean $useLogNormal
setParam FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 useLogNormal boolean $useLogNormal
setParam FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 useLogNormal boolean $useLogNormal
setParam FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 useLogNormal boolean $useLogNormal
setParam FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 useLogNormal boolean $useLogNormal
setParam FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 useLogNormal boolean $useLogNormal
setParam FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 useLogNormal boolean $useLogNormal
setParam FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 useLogNormal boolean $useLogNormal

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
sendSimpleCmdToApp FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 Configure
sendSimpleCmdToApp FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 Configure
sendSimpleCmdToApp FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 Configure
sendSimpleCmdToApp FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 Configure
sendSimpleCmdToApp FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 Configure
sendSimpleCmdToApp FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 Configure
sendSimpleCmdToApp FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 Configure
sendSimpleCmdToApp FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 Configure
sendSimpleCmdToApp FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 Configure
sendSimpleCmdToApp FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 Configure
sendSimpleCmdToApp FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 Configure
sendSimpleCmdToApp FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 Configure
sendSimpleCmdToApp FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 Configure
sendSimpleCmdToApp FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 Configure
sendSimpleCmdToApp FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 Configure
sendSimpleCmdToApp FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 Configure
sendSimpleCmdToApp FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 Configure
sendSimpleCmdToApp FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 Configure
sendSimpleCmdToApp FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 Configure
sendSimpleCmdToApp FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 Configure

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
sendSimpleCmdToApp FEROL12_SOAP_HOST_NAME FEROL12_SOAP_PORT evb::test::DummyFEROL 12 Enable
sendSimpleCmdToApp FEROL13_SOAP_HOST_NAME FEROL13_SOAP_PORT evb::test::DummyFEROL 13 Enable
sendSimpleCmdToApp FEROL14_SOAP_HOST_NAME FEROL14_SOAP_PORT evb::test::DummyFEROL 14 Enable
sendSimpleCmdToApp FEROL15_SOAP_HOST_NAME FEROL15_SOAP_PORT evb::test::DummyFEROL 15 Enable
sendSimpleCmdToApp FEROL16_SOAP_HOST_NAME FEROL16_SOAP_PORT evb::test::DummyFEROL 16 Enable
sendSimpleCmdToApp FEROL17_SOAP_HOST_NAME FEROL17_SOAP_PORT evb::test::DummyFEROL 17 Enable
sendSimpleCmdToApp FEROL18_SOAP_HOST_NAME FEROL18_SOAP_PORT evb::test::DummyFEROL 18 Enable
sendSimpleCmdToApp FEROL19_SOAP_HOST_NAME FEROL19_SOAP_PORT evb::test::DummyFEROL 19 Enable
sendSimpleCmdToApp FEROL20_SOAP_HOST_NAME FEROL20_SOAP_PORT evb::test::DummyFEROL 20 Enable
sendSimpleCmdToApp FEROL21_SOAP_HOST_NAME FEROL21_SOAP_PORT evb::test::DummyFEROL 21 Enable
sendSimpleCmdToApp FEROL22_SOAP_HOST_NAME FEROL22_SOAP_PORT evb::test::DummyFEROL 22 Enable
sendSimpleCmdToApp FEROL23_SOAP_HOST_NAME FEROL23_SOAP_PORT evb::test::DummyFEROL 23 Enable
sendSimpleCmdToApp FEROL24_SOAP_HOST_NAME FEROL24_SOAP_PORT evb::test::DummyFEROL 24 Enable
sendSimpleCmdToApp FEROL25_SOAP_HOST_NAME FEROL25_SOAP_PORT evb::test::DummyFEROL 25 Enable
sendSimpleCmdToApp FEROL26_SOAP_HOST_NAME FEROL26_SOAP_PORT evb::test::DummyFEROL 26 Enable
sendSimpleCmdToApp FEROL27_SOAP_HOST_NAME FEROL27_SOAP_PORT evb::test::DummyFEROL 27 Enable
sendSimpleCmdToApp FEROL28_SOAP_HOST_NAME FEROL28_SOAP_PORT evb::test::DummyFEROL 28 Enable
sendSimpleCmdToApp FEROL29_SOAP_HOST_NAME FEROL29_SOAP_PORT evb::test::DummyFEROL 29 Enable
sendSimpleCmdToApp FEROL30_SOAP_HOST_NAME FEROL30_SOAP_PORT evb::test::DummyFEROL 30 Enable
sendSimpleCmdToApp FEROL31_SOAP_HOST_NAME FEROL31_SOAP_PORT evb::test::DummyFEROL 31 Enable

echo "Sending data for 10 seconds"
sleep 10

expectedEventSize=$((16*($fragmentSize+16))) #add FEROL header size

superFragmentSizeEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt`
superFragmentSizeRU1=`getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 1 superFragmentSize xsd:unsignedInt`

echo "EVM superFragmentSize: $superFragmentSizeEVM"
echo "RU1 superFragmentSize: $superFragmentSizeRU1"

if [[ $superFragmentSizeEVM -ne $expectedEventSize ]]
then
  echo "Test failed: expected $expectedEventSize"
  exit 1
fi

if [[ $superFragmentSizeRU1 -ne $expectedEventSize ]]
then
  echo "Test failed: expected $expectedEventSize"
  exit 1
fi

eventRateEVM=`getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt`
echo "EVM eventRate: $eventRateEVM"

if [[ $eventRateEVM -lt 100000 ]]
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
