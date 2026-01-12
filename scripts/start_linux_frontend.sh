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
    $frontend -h $midas_host:$midas_port >| $DAQ_OUTPUT_TOP/logs/$frontend/$frontend.$local_node.log 2>&1 &
else
    cmd="export MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
    cmd=$cmd"; cd $MU2E_DAQ_DIR"
    cmd=$cmd"; source setup_daq.sh $spack_env"
    cmd=$cmd"; logdir=$DAQ_OUTPUT_TOP/logs/$frontend; if [ ! -d $logdir ] ; then mkdir -p $logdir ; fi"
    cmd=$cmd"; nohup $frontend -h $midas_host:$midas_port >| $DAQ_OUTPUT_TOP/logs/$frontend/$frontend.$local_node.log 2>&1 &"

    if [ $verbose != 0 ] ; then echo ssh -KX $USER@$remote_node.fnal.gov $cmd ; fi
    ssh -KX $USER@$remote_node.fnal.gov  $cmd
fi
