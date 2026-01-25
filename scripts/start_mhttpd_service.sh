#!/usr/bin/bash
#------------------------------------------------------------------------------
# to be installed in ~/bin/
#------------------------------------------------------------------------------
cd $HOME/daquser_002   # current $MU2E_DAQ_DIR, update if $MU2E_DAQ_DIR changes
source ./setup_daq.sh
mhttpd -e $MIDAS_EXPT_NAME
