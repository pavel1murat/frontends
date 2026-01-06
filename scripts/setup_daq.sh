#!/usr/bin/bash
#------------------------------------------------------------------------------
# this script is also executed by the ARTDAQ processes running on different nodes
# those are not supposed to use any arguments and use the default spack environment
#------------------------------------------------------------------------------
spack_env=v001
if [ ".$1" != "." ] ; then spack_env=$1 ; fi

echo ... setting up environment $spack_env

if [ -z $MU2E_DAQ_DIR ] ; then
    export MU2E_DAQ_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
fi
#------------------------------------------------------------------------------
# ensure uniform (short) interpretation of the hostname
#------------------------------------------------------------------------------
export      HISTTIMEFORMAT="%d/%m/%y %T "
export            HOSTNAME=`hostname -s`
export       DATA_DIR_STUB=`echo $MU2E_DAQ_DIR | awk -F / '{print $NF}'`_$spack_env
export       DAQ_USER_STUB=${USER}_`echo $MU2E_DAQ_DIR | awk -F / '{print $NF}'`
# 
export      DAQ_OUTPUT_TOP=/scratch/mu2e/$USER/$DATA_DIR_STUB
export        RAW_DATA_DIR=$DAQ_OUTPUT_TOP/data

unset SPACK_ROOT
export SPACK_DISABLE_LOCAL_CONFIG=true
source $MU2E_DAQ_DIR/spack/share/spack/setup-env.sh
spack env activate $spack_env   # murat tdaq
export  SPACK_VIEW=$SPACK_ENV/.spack-env/view
#------------------------------------------------------------------------------
# build in the env area, not on /tmp
#------------------------------------------------------------------------------
export         TMP=$SPACK_ENV/build
export      TMPDIR=$SPACK_ENV/build

                  namestub=${USER}_`pwd | awk -F / '{print $NF}'` # example: mu2etrk_pasha_028
# export TFM_FHICL_DIRECTORY=$MU2E_DAQ_DIR/config  # no longer needed

            export TFM_DIR=$SPACK_ENV/tfm
      export FRONTENDS_DIR=$SPACK_END/frontends

      export MIDASSYS=$SPACK_VIEW
if [ $spack_env == "v001" ] ; then 
    export MIDAS_EXPT_NAME=tracker
elif [ $spack_env == "namitha" ] ; then
    export MIDAS_EXPT_NAME=namitha
fi
    export    MIDAS_EXPTAB=$PWD/config/midas/mc2.exptab

# P.M. need to get rid of ARTDAQ_PARTITION_NUMBER here, not there yet
export    ARTDAQ_PARTITION_NUMBER=11
export ARTDAQ_PORTS_PER_PARTITION=1000
export           ARTDAQ_BASE_PORT=10000
#------------------------------------------------------------------------------
# STM specific variables
#------------------------------------------------------------------------------
export   STM_HPGE_SW_IP="127.0.0.2" # HPGe (CH0) UDP IP address (send)
export   STM_LABR_SW_IP="127.0.0.3" # LaBr (CH1) UDP IP address (send)
export STM_HPGE_SW_PORT=10010 # HPGe (CH0) UDP IP port (send)
export STM_LABR_SW_PORT=10012 # LaBr (CH1) UDP IP port (send)
#------------------------------------------------------------------------------
# TRACE - some defaults
#------------------------------------------------------------------------------
export                TRACE_FILE=/tmp/trace_file.$USER.`spack env status | awk '{print $NF}'`
export            TRACE_LIMIT_MS=0,50,50
export              TRACE_MSGMAX=0        # Activating TRACE

tonMg  0-4   # enable trace to memory
tonSg  0-7   # enable trace to slow path (i.e. UDP)
toffSg 8-63  # apparently not turned off by default?
#------------------------------------------------------------------------------
# interfaces to mu2e_pcie_utils
#------------------------------------------------------------------------------
export   BUILD_ROOT_INTERFACE=1
export BUILD_PYTHON_INTERFACE=0           # not yet ready
#------------------------------------------------------------------------------
# TFM - don't think it is currently used 
#------------------------------------------------------------------------------
export TFM_CONFIG_DIR=$MU2E_DAQ_DIR/config/artdaq
#------------------------------------------------------------------------------
# paths
#------------------------------------------------------------------------------
export             PATH=$PATH:$MU2E_DAQ_DIR/config/scripts
export       PYTHONPATH=$SPACK_VIEW/python
export MU2E_SEARCH_PATH=$SPACK_ENV:/cvmfs/mu2e.opensciencegrid.org/DataFiles
export  FHICL_FILE_PATH=$SPACK_ENV:$FHICL_FILE_PATH
export  LD_LIBRARY_PATH=$SPACK_VIEW/lib

return 0
