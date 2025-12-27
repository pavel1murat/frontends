#!/usr/bin/bash
#------------------------------------------------------------------------------
# node_frontend processes are running on a network tagged as 'PrivateNetwork'
# in some cases it can be the same as the public network
#------------------------------------------------------------------------------
remote_node=$1
    verbose=0; if [ ".$2" != "." ] ; then verbose=$2 ; fi
#------------------------------------------------------------------------------
# start dtc_frontend on a remote node
# /Mu2e/ActiveRunConfiguration/DAQ/LocalSubnet defines the local subnet IP 
#------------------------------------------------------------------------------
local_node=`hostname -s`

frontend=node_frontend
local_subnet=`odbedit -q -c 'ls -v /Mu2e/ActiveRunConfiguration/DAQ/PrivateSubnet'`

if [ $verbose != 0 ] ; then echo LINENO:$LINENO local_subnet:$local_subnet ; fi

function hostname_on_subnet() {
    ifconfig -a | grep $1 | awk '{print $2}' | nslookup | grep -v ipmi | head -n 1 | \
        sed 's/\\.$//' | awk '{print $NF}'
}

# midas_host=mu2edaq09-ctrl.fnal.gov
midas_host=`hostname_on_subnet $local_subnet`

echo LINENO:$LINENO verbose=$verbose remote_mode=$remote_node midas_host:$midas_host

if   [ $remote_node == $local_node ] ; then
    $frontend -h $midas_host &
else
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
    cmd=$cmd"; source setup_daq.sh"
    cmd=$cmd"; $frontend -h $midas_host"
#    cmd=$cmd"&"
    if [ $verbose != 0 ] ; then echo ssh -KX $USER@$remote_node.fnal.gov $cmd ; fi
    ssh -KX $USER@$remote_node.fnal.gov  $cmd
fi
