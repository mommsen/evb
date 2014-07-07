#!/bin/sh

rm -f /tmp/dump_run000000_event*txt

function sendCmdToFEROL
{
  sendSimpleCmdToApp FEROL0_SOAP_HOST_NAME FEROL0_SOAP_PORT evb::test::DummyFEROL 0 $1
  sendSimpleCmdToApp FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 $1
  sendSimpleCmdToApp FEROL2_SOAP_HOST_NAME FEROL2_SOAP_PORT evb::test::DummyFEROL 2 $1
  sendSimpleCmdToApp FEROL3_SOAP_HOST_NAME FEROL3_SOAP_PORT evb::test::DummyFEROL 3 $1
  sendSimpleCmdToApp FEROL4_SOAP_HOST_NAME FEROL4_SOAP_PORT evb::test::DummyFEROL 4 $1
  sendSimpleCmdToApp FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 $1
  sendSimpleCmdToApp FEROL6_SOAP_HOST_NAME FEROL6_SOAP_PORT evb::test::DummyFEROL 6 $1
  sendSimpleCmdToApp FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 $1
}

function sendCmdToEvB
{
  sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 $1
  sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 $1
  sleep 1
  sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 $1
}

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
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Configure
sendSimpleCmdToApp RU0_SOAP_HOST_NAME RU0_SOAP_PORT pt::utcp::Application 0 Enable
sendSimpleCmdToApp RU1_SOAP_HOST_NAME RU1_SOAP_PORT pt::utcp::Application 1 Enable
sendSimpleCmdToApp BU0_SOAP_HOST_NAME BU0_SOAP_PORT pt::utcp::Application 2 Enable

# Configure all applications
sendCmdToEvB Configure
sendCmdToFEROL Configure
sleep 1

#Enable the system
sendCmdToEvB Enable
sendCmdToFEROL Enable

echo "Sending data..."
sleep 1

echo "Corrupt an event on FED 1"
setParam FEROL1_SOAP_HOST_NAME FEROL1_SOAP_PORT evb::test::DummyFEROL 1 corruptNbEvents unsignedInt 1

sleep 2

echo "Corrupt 3 events on FED 5"
setParam FEROL5_SOAP_HOST_NAME FEROL5_SOAP_PORT evb::test::DummyFEROL 5 corruptNbEvents unsignedInt 3

sleep 2

echo "Corrupt 2 events on FED 7"
setParam FEROL7_SOAP_HOST_NAME FEROL7_SOAP_PORT evb::test::DummyFEROL 7 corruptNbEvents unsignedInt 2

sleep 3

state=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 stateName xsd:string)
echo "EVM state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 stateName xsd:string)
echo "RU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

state=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 stateName xsd:string)
echo "BU state=$state"
if [[ "$state" != "Enabled" ]]
then
  echo "Test failed"
  exit 1
fi

superFragmentSizeEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 superFragmentSize xsd:unsignedInt)
echo "EVM superFragmentSize: $superFragmentSizeEVM"
if [[ $superFragmentSizeEVM -ne 8192 ]]
then
  echo "Test failed: expected 8192"
  exit 1
fi

superFragmentSizeRU1=$(getParam RU1_SOAP_HOST_NAME RU1_SOAP_PORT evb::RU 0 superFragmentSize xsd:unsignedInt)
echo "RU1 superFragmentSize: $superFragmentSizeRU1"
if [[ $superFragmentSizeRU1 -ne 8192 ]]
then
  echo "Test failed: expected 8192"
  exit 1
fi

eventRateEVM=$(getParam RU0_SOAP_HOST_NAME RU0_SOAP_PORT evb::EVM 0 eventRate xsd:unsignedInt)
echo "EVM eventRate: $eventRateEVM"
if [[ $eventRateEVM -lt 500 ]]
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
if [[ $eventSizeBU0 -ne 16384 ]]
then
  echo "Test failed: expected 16384"
  exit 1
fi

nbCorruptedEventsBU0=$(getParam BU0_SOAP_HOST_NAME BU0_SOAP_PORT evb::BU 0 nbCorruptedEvents xsd:unsignedLong)
echo "BU0 nbCorruptedEvents: $nbCorruptedEventsBU0"
if [[ $nbCorruptedEventsBU0 -ne 6 ]]
then
  echo "Test failed: expected 6"
  exit 1
fi

nbFedDumps=$(find /tmp -regex '/tmp/dump_run000000_event[0-9]+_fed000[157].txt'|wc -l)
echo "Nb of FED dumps=$nbFedDumps"
if [[ $nbFedDumps -ne 6 ]]
then
  echo "Test failed"
  exit 1
fi

nbEventDumps=$(find /tmp -regex '/tmp/dump_run000000_event[0-9]+.txt'|wc -l)
echo "Nb of event dumps=$nbEventDumps"
if [[ $nbEventDumps -ne 6 ]]
then
  echo "Test failed"
  exit 1
fi

echo "Test launched successfully"
exit 0
