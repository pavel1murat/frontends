#!/usr/bin/bash

remote_node=$1
#------------------------------------------------------------------------------
# start dtc_frontend on a remote node
#------------------------------------------------------------------------------
if [ $remote_node == "mu2edaq22" ] ; then
#------------------------------------------------------------------------------
# 1. daq22:
#------------------------------------------------------------------------------
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
    cmd=$cmd"; source setup_daq.sh"
    cmd=$cmd"; dtc_frontend -h `hostname -s`-ctrl.fnal.gov"
    cmd=$cmd"&"
    ssh mu2etrk@$remote_node.fnal.gov  $cmd
fi
