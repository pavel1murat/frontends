#!/usr/bin/bash
#------------------------------------------------------------------------------
# reprogram DTCs on a current node
# current defautl DTC FW version is DTC2026Jan16_17_22.1_6ROC
#------------------------------------------------------------------------------
bitfile=/home/mu2ehwdev/DTC_Firmware/DTC2026Jan16_17_22.1_6ROC/DTC.bit

source /home/xilinx/Vivado_Lab/2021.2/settings64.sh
vivado_lab -mode batch -source ~mu2ehwdev/ots_spack/srcs/otsdaq-mu2e/tools/program_both_DTCs.tcl -tclargs $bitfile $bitfile
rc=$?
echo rc=$rc
if [ $rc != 0 ] ; then
    echo failed, exiting
    return
fi

sudo /home/mu2ehwdev/reset_PCIe_AL9.sh
