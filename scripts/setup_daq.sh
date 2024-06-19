#!/usr/bin/bash

export        MU2E_DAQ_DIR=$PWD;
export       DAQ_USER_STUB=${USER}_`echo $MU2E_DAQ_DIR | awk -F / '{print $NF}'`
export      DAQ_OUTPUT_TOP=/scratch/mu2e/$DAQ_USER_STUB

source $MU2E_DAQ_DIR/spack/share/spack/setup-env.sh
spack env activate tdaq-v3_01_00

export          TRACE_FILE=/tmp/mu2etrk_pasha_028.$$
export TFM_FHICL_DIRECTORY=$MU2E_DAQ_DIR/config

            export TFM_DIR=$MU2E_DAQ_DIR/tdaq-v3_01_00/tfm
      export FRONTENDS_DIR=$MU2E_DAQ_DIR/tdaq-v3_01_00/frontends

        export  SPACK_VIEW=$MU2E_DAQ_DIR/tdaq-v3_01_00/.spack-env/view
        export  PYTHONPATH=$SPACK_VIEW/python

           export MIDASSYS=$MU2E_DAQ_DIR/tdaq-v3_01_00/.spack-env/view
    export MIDAS_EXPT_NAME=test_025
    export    MIDAS_EXPTAB=$PWD/config/$MIDAS_EXPT_NAME.exptab

export    ARTDAQ_PARTITION_NUMBER=11
export ARTDAQ_PORTS_PER_PARTITION=1000
export           ARTDAQ_BASE_PORT=21000
#------------------------------------------------------------------------------
# trace
#------------------------------------------------------------------------------
export                TRACE_FILE=/tmp/trace_file.$$
export            TRACE_LIMIT_MS=0,50,50
export              TRACE_MSGMAX=0        # Activating TRACE

tonMg  0-4   # enable trace to memory
tonSg  0-7   # enable trace to slow path (i.e. UDP)
toffSg 8-63  # apparently not turned off by default?
#------------------------------------------------------------------------------
# TFM
#------------------------------------------------------------------------------
export TFM_CONFIG_DIR=$MU2E_DAQ_DIR/config
return 0
