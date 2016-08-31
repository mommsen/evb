#!/bin/sh

#swt=/DAQ2_Official/StandAlone/StandAlone_40g_infini_dropAtBU
swt=/Test/Remi/StandAlone/StandAlone_40g_infini_dropAtBU
hwcfg=/daq2val/eq_160316_emu
dir=../daq2val_160829
mkdir -p $dir

(
cat <<'EOF'
RUEVM_CHECK_CRC 0
BU_CHECK_CRC 0
BU_CALC_CRC32C false
EOF
) > /tmp/$$.txt

for setting in '4s4f' '8s8f' '12s12f' '16s16f' '24s24f' '4s2f' '8s4f' '12s6f' '16s8f' '24s12f' ; do
#for setting in '16s16f' ; do
    ./createConfig.py $hwcfg/1fb_$setting $swt 2 $dir/${setting}x1x2_noChksums -s /tmp/$$.txt --daqval -p $HOME/configurator/CONFIGURATOR_DAQ2VAL.properties
    ./createConfig.py $hwcfg/1fb_$setting $swt 2 $dir/${setting}x1x2 --daqval -p $HOME/configurator/CONFIGURATOR_DAQ2VAL.properties
done

#./createConfig.py $hwcfg/3fb_8s8f $swt 2 $dir/3x8s8f_noChksums -s /tmp/$$.txt --daqval -p $HOME/configurator/CONFIGURATOR_DAQ2VAL.properties

rm -f /tmp/$$.txt
