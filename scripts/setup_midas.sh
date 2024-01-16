#!/usr/bin/bash
#------------------------------------------------------------------------------
# call signature: source setup_midas.sh test_001
# test_001: experiment name
# ROOT is supposed to be setup
#------------------------------------------------------------------------------
export MIDAS_EXPT_NAME=$1

export        MIDASSYS=/home/mu2etrk/test_stand/pasha_020/midas
export    MIDAS_EXPTAB=/home/mu2etrk/test_stand/pasha_020/config/$MIDAS_EXPT_NAME.exptab

export            PATH=$PATH:$MIDASSYS/bin
