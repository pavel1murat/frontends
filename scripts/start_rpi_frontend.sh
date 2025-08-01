#!/usr/bin/bash
#------------------------------------------------------------------------------
# call : config/scripts/start_rpi_frontend.sh <rpi>
#
# rpi: the name of the RPI - has to be present
#      currently: trackerpi11(EC), trackerpi12(Lab3)
#------------------------------------------------------------------------------
remote_node=$1
    verbose=0; if [ ".$2" != "." ] ; then verbose=$2 ; fi
#------------------------------------------------------------------------------
# start PI frontend on a remote node
# the node names may depend on the subnet
# pi's communicate over the public network
#------------------------------------------------------------------------------
local_node=`hostname -s`

frontend=rpi_frontend.py
midas_host=`hostname -s`.fnal.gov

echo LINENO=$LINENO verbose=$verbose remote_mode=$remote_node

#------------------------------------------------------------------------------
cmd='cd /home/mu2e/daq; echo $PWD'
cmd=$cmd'; source setup_midas.sh'
cmd=$cmd'; export PYTHONPATH=$PYTHONPATH:$HOME/LVHVBox/Client; echo PYTHONPATH=$PYTHONPATH'
cmd=$cmd"; python3 $frontend -h $midas_host &"

if [ $verbose != 0 ] ; then echo $cmd ; fi
ssh -K mu2e@$remote_node.dhcp.fnal.gov  $cmd 
