//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __cfo_frontend_hh__
#define __cfo_frontend_hh__

#include "drivers/bus/null.h"

#include "frontends/utils/mu2e_sc.hh"

// #include "frontends/cfo_frontend/cfo_driver.hh"

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {"CFO",                      // equipment name, to become mu2edaq09::DTC
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   // equipment type. PM: make everything slow , we're not reading the data 
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_RUNNING,                // RO_TRANSITIONS, // | RO_RUNNING , // RO_ALWAYS,
    5000,                      // period, msec if 0 do not do anything .. 10000
                               // P.M. for EQ_SLOW, should this number be sync'ed with 
                               // the log_history every number ?
    0,                         /* event_limit */
    0,                         /* number of sub events */
    30,                        // 'history' log history at most every 30 seconds - P.M. validated
    "",                        // host name
    "",                        // frontend name
    "",                        // frontend source file name
    "",                        // current status
    "",                        // color for status
    0,                         // hidden flag
    0                          // write cache size
   } ,
   cd_mu2e_sc_read,            /* readout routine */
   cd_mu2e_sc,                 /* class driver main routine */
   //   driver_list,           // device driver list
   nullptr,                    // driver (driver list) initialized in frontend_init
   NULL,                       /* init string */
  },
  {""}
};

#endif
