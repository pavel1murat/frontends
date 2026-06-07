#!/usr/bin/bash
#------------------------------------------------------------------------------
# call signature:
#                 config/scripts/start_linux_frontend,sh frontend node [v]
# example:
#                 config/scripts/start_linux_frontend,sh node_fronend mu2e-trk-09
#
# v : verbose
#
# node_frontend processes are running on a network defined as 'PrivateNetwork'
# in some cases it can be the same as the public network
#------------------------------------------------------------------------------
   frontend=$1
remote_node=$2
    verbose=0; if [ ".$3" != "." ] ; then verbose=$3 ; fi
#------------------------------------------------------------------------------
# start dtc_frontend on a remote node
# /Mu2e/ActiveRunConfiguration/DAQ/LocalSubnet defines the local subnet IP 
#------------------------------------------------------------------------------
  local_node=`hostname -s`
local_subnet=`odbedit -q -c 'ls -v /Mu2e/ActiveRunConfiguration/DAQ/PrivateSubnet'`
  midas_port=`odbedit -q -c 'ls -v "/Experiment/Midas server port"'`

if [ $verbose != 0 ] ; then echo LINENO:$LINENO local_subnet:$local_subnet ; fi

function hostname_on_subnet() {
    ifconfig -a | grep $1 | awk '{print $2}' | nslookup | grep -v ipmi | head -n 1 | \
        sed 's/\\.$//' | awk '{print $NF}'
}

midas_host=`hostname_on_subnet $local_subnet`
midas_host=${midas_host%?}
 spack_env=`echo $SPACK_ENV | awk -F/ '{print $NF}'`

echo LINENO:$LINENO verbose=$verbose remote_mode=$remote_node midas_host:$midas_host

if   [ $remote_node == $local_node ] ; then
    logdir=$DAQ_OUTPUT_TOP/logs/$frontend; if [ ! -d $logdir ] ; then mkdir -p $logdir ; fi
    log_fn=$frontend.$remote_node.`date +%Y-%m-%d-%H-%M-%S`.log
    nohup $frontend -h $midas_host:$midas_port >| $DAQ_OUTPUT_TOP/logs/$frontend/$log_fn 2>&1 &
else
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
#------------------------------------------------------------------------------
# whenever the setup_daq.sh gets modified, quick_setup.sh is generated when TFM restarts
# quick_setup.sh is used by start_linux_frontend.sh and by start_artdaq_processes.sh
#------------------------------------------------------------------------------
    cmd=$cmd"; source config/scripts/quick_setup.sh"
    cmd=$cmd"; logdir=$DAQ_OUTPUT_TOP/logs/$frontend; if [ ! -d \$logdir ] ; then mkdir -p \$logdir ; fi"
    cmd=$cmd"; logfile=$frontend.$remote_node.`date +%Y-%m-%d-%H-%M-%S`.log"
    cmd=$cmd"; nohup $frontend -h $midas_host:$midas_port"
    
    if [ $verbose != 0 ] ; then echo ssh -KX $USER@$remote_node.fnal.gov \"$cmd \>\| \$logdir/\$logfile 2\>\&1\" ; fi

    ssh -KX $USER@$remote_node.fnal.gov  "$cmd >| \$logdir/\$logfile 2>&1 &"
fi
