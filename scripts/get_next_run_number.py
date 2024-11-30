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
    client = midas.client.MidasClient("get_next_run", "mu2edaq22-ctrl", "test_025", None)
    rundb  = fur.RuninfoDB(client)

    try:
        next_run = rundb.next_run_number(store_in_odb=True)
    except Exception as e:
        TRACE.ERROR("FAILED TO GET NEXT RUN NUMBER")

    return next_run;

#------------------------------------------------------------------------------
if __name__ == "__main__":
    rn = get_next_run_number()
    TRACE.DEBUG(f'NEXT RUN NUMBER:{rn}')
