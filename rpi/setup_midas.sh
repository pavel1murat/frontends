#!/usr/bin/bash
#------------------------------------------------------------------------------
# PM: to be executed on a Raspberry PI
# TODO: make sure that MIDASSYS is the same on all PI's
#------------------------------------------------------------------------------
export MIDASSYS=/opt/midas

export            PATH=$PATH:$MIDASSYS/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MIDASSYS/lib
export      PYTHONPATH=$PYTHONPATH:$MIDASSYS/python:$HOME/LVHVBox/Client
