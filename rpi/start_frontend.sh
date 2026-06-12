#!/bin/bash
  frontend=$1
midas_host=$2
experiment=$3

cd /home/mu2e/daq;
echo $PWD frontend=$frontend midas_host=$midas_host experiment=$experiment
source ./setup_midas.sh
export PYTHONPATH=$PYTHONPATH:$HOME/LVHVBox/Client; echo PYTHONPATH=$PYTHONPATH
python3 $frontend -h $midas_host -e $experiment
