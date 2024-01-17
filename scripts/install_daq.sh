#!/usr/bin/bash
#------------------------------------------------------------------------------
# assume it is submitted trom the top directory of the DAQ work area,
# the one where mhttpd is submitted from
#------------------------------------------------------------------------------
mkdir daq_scripts

cp srcs/frontends/scripts/setup_ots.sh   .
cp srcs/frontends/scripts/setup_midas.sh .

cp    srcs/frontends/scripts/get_output_file_size daq_scripts/.
cp    srcs/frontends/scripts/start_farm_manager   daq_scripts/.
