#!/usr/bin/bash
#------------------------------------------------------------------------------
# emulate getting the next run number from the DB 
# call signature: 
# ---------------
#                      get_next_run_number.sh run_configuration
#
# run_configuration : the name the run configuration, for which a new run is requested
#------------------------------------------------------------------------------
# echo 77
run_configuration=$1
get_next_run_number $run_configuration
