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
sendCmdToLauncher RU0_SOAP_HOST_NAME RU0_LAUNCHER_PORT STARTXDAQRU0_SOAP_PORT
sendCmdToLauncher RU1_SOAP_HOST_NAME RU1_LAUNCHER_PORT STARTXDAQRU1_SOAP_PORT
sendCmdToLauncher RU2_SOAP_HOST_NAME RU2_LAUNCHER_PORT STARTXDAQRU2_SOAP_PORT
sendCmdToLauncher BU0_SOAP_HOST_NAME BU0_LAUNCHER_PORT STARTXDAQBU0_SOAP_PORT

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

# Configure all executives
sendCmdToExecutive FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT configure.cmd.xml
sendCmdToExecutive FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU0_SOAP_HOST_NAME RU0_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU1_SOAP_HOST_NAME RU1_SOAP_PORT configure.cmd.xml
sendCmdToExecutive RU2_SOAP_HOST_NAME RU2_SOAP_PORT configure.cmd.xml
sendCmdToExecutive BU0_SOAP_HOST_NAME BU0_SOAP_PORT configure.cmd.xml

# Configure and enable pt
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Configure
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::frl::Application 4 Configure
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::frl::Application 5 Configure
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::frl::Application 6 Configure
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::frl::Application 7 Configure
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT pt::frl::Application 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT pt::frl::Application 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT pt::frl::Application 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT pt::frl::Application 3 Enable
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT pt::frl::Application 4 Enable
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT pt::frl::Application 5 Enable
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT pt::frl::Application 6 Enable
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT pt::frl::Application 7 Enable

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 3 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT pt::utcp::Application 2 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 3 Enable

# Configure all applications
sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Configure
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Configure
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Configure
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Configure
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 Configure
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 Configure
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 Configure
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 Configure

sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Configure
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Configure
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Configure
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Configure
sleep 2

#Enable the system
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 Enable
sendSimpleCmdToApp RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 Enable

sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 Enable
sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 Enable
sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 Enable
sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 Enable
sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 Enable
sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 Enable
sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 Enable
sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 Enable

echo "Sending data for 5 seconds"
sleep 5

superFragmentSizeEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt)
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne 9232 ]]
then
  echo "Test failed: expected 9232"
  exit 1
fi

superFragmentSizeRU1=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt)
echo "RU1 superFragmentSize: $superFragmentSizeRU1"
if [[ $superFragmentSizeRU1 -ne 22832 ]]
then
  echo "Test failed: expected 22832"
  exit 1
fi

superFragmentSizeRU2=$(getParam RU2_SOAP_HOST_NAME RU2_SOAP_PORT evb::RU 1 superFragmentSize xsd:unsignedInt)
echo "RU2 superFragmentSize: $superFragmentSizeRU2"
if [[ $superFragmentSizeRU2 -ne 4080 ]]
then
  echo "Test failed: expected 4080"
  exit 1
fi

eventRateEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt)
echo "EVM eventRate: $eventRateEVM"
if [[ $eventRateEVM -lt 500 ]]
then
  echo "Test failed"
  exit 1
fi

eventCountEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventCount xsd:unsignedLong)
echo "EVM eventCount: $eventCountEVM"
if [[ $eventCountEVM -lt 500 ]]
then
  echo "Test failed"
  exit 1
fi

nbEventsBuiltBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbEventsBuilt xsd:unsignedLong)
echo "BU0 nbEventsBuilt: $nbEventsBuiltBU0"
if [[ $nbEventsBuiltBU0 -lt 1000 ]]
then
  echo "Test failed"
  exit 1
fi

eventSizeBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 eventSize xsd:unsignedInt)
echo "BU0 eventSize: $eventSizeBU0"
if [[ $eventSizeBU0 -ne 36144 ]]
then
  echo "Test failed: expected 36144"
  exit 1
fi

echo "Test launched successfully"
exit 0
