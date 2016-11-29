# XDAQ
export XDAQ_ROOT=/opt/xdaq

export XDAQ_PLATFORM=`uname -m`
if test ".$XDAQ_PLATFORM" != ".x86_64"; then
    export XDAQ_PLATFORM=x86
fi
checkos=`$XDAQ_ROOT/config/checkos.sh`
export XDAQ_PLATFORM=$XDAQ_PLATFORM"_"$checkos
export XDAQ_EVB=${HOME}/daq/baseline13/${XDAQ_PLATFORM}
export LD_LIBRARY_PATH=${XDAQ_EVB}/lib:${LD_LIBRARY_PATH}

# EvB tester suite
export EVB_TESTER_HOME=${HOME}/daq/baseline13/daq/evb/test
export EVB_SYMBOL_MAP=standaloneSymbolMap.txt
export PATH=${PATH}:${EVB_TESTER_HOME}/scripts
export PYTHONPATH=${EVB_TESTER_HOME}/scripts:${PYTHONPATH}

ulimit -l unlimited
