#!/usr/bin/env python
###############################################################################
# 2024-05-11 P.M. 
# - the ODB connection should be established by the caller
# - the same is true for disconnecting from ODB
# - use midas.client
###############################################################################
import  os, midas.client, logging
import  frontends.utils.runinfodb as fur;

import TRACE
TRACE_NAME = 'get_next_run_number'

logger = logging.getLogger('midas')

#------------------------------------------------------------------------------
# the hostname and the experiment name need to come from the environment
#------------------------------------------------------------------------------
def get_next_run_number():
    experiment   = os.getenv("MIDAS_EXPT_NAME")
    client       = midas.client.MidasClient("get_next_run_number", "localhost",experiment, None)
    mu2e_daq_dir = os.path.expandvars(os.getenv("MU2E_DAQ_DIR"))
    cfg_file     =  mu2e_daq_dir+'/config/runinfodb.json';
    rundb        = fur.RuninfoDB(midas_client=client,config_file=cfg_file);

    try:
        next_run = rundb.next_run_number(store_in_odb=True)
        return next_run;
    except Exception as e:
        TRACE.ERROR("FAILED TO GET NEXT RUN NUMBER")
        return -1;

#------------------------------------------------------------------------------
if __name__ == "__main__":
    rn = get_next_run_number()
    msg=f'NEXT RUN NUMBER:{rn}'
    TRACE.TRACE(6,msg)
