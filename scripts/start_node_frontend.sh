#!/usr/bin/bash

remote_node=$1
    verbose=0; if [ ".$2" != "." ] ; then verbose=$2 ; fi
#------------------------------------------------------------------------------
# start dtc_frontend on a remote node
# the node names may depend on the subnet
#------------------------------------------------------------------------------
local_node=`hostname -s`

frontend=dtc_mt_frontend
midas_host=`hostname -s`-ctrl.fnal.gov

echo LINENO=$LINENO verbose=$verbose remote_mode=$remote_node

if   [ $remote_node == $local_node ] ; then
    $frontend &
else
#------------------------------------------------------------------------------
# 1. mu2edaq22 --> mu2edaq22-ctrl (assume Edwards Center)
#------------------------------------------------------------------------------
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
    cmd=$cmd"; source setup_daq.sh"
    cmd=$cmd"; $frontend -h $midas_host"
#    cmd=$cmd"&"
    if [ $verbose != 0 ] ; then echo $cmd ; fi
    ssh -KX mu2etrk@$remote_node.fnal.gov  $cmd
fi
