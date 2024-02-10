#!/usr/bin/bash
#------------------------------------------------------------------------------
# assume it is submitted trom the top directory of the DAQ work area,
# the one where mhttpd is submitted from
#------------------------------------------------------------------------------
function install_daq_() {
    top=$MRB_TOP
    mkdir $MRB_TOP/daq_scripts

    cp srcs/frontends/scripts/setup_ots.sh   $top/.
    cp srcs/frontends/scripts/setup_midas.sh $top/.

    cp    srcs/frontends/scripts/get_output_file_size $top/daq_scripts/.
    cp    srcs/frontends/scripts/start_farm_manager   $top/daq_scripts/.
}

install_daq_ 
