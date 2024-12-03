#!/usr/bin/bash

remote_node=$1
#------------------------------------------------------------------------------
# start trk DTC control on a remote node
# the node names may depend on the subnet
#------------------------------------------------------------------------------
local_node=`hostname -s`

frontend=trk_dtc_ctl_frontend

if   [ $remote_node == $local_node ] ; then
    $frontend &
elif [ $remote_node == "mu2edaq22" ] ; then
#------------------------------------------------------------------------------
# 1. mu2edaq22 --> mu2edaq22-ctrl (assume Edwards Center)
#------------------------------------------------------------------------------
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
    cmd=$cmd"; source setup_daq.sh"
    cmd=$cmd"; $frontend -h `hostname -s`-ctrl.fnal.gov"
    cmd=$cmd"&"
    ssh mu2etrk@$remote_node.fnal.gov  $cmd
fi
