//////////////////////////////////////////////////////////////////////////////
// CFO: need two drivers: one for generating EWMs, another one - for slow monitoring
// thus, two pieces of slow equipment : CFO and CFOMon, polled at different rate
// an emulated CFO generated EWMs just once - at begin run,
// and needs only a monitoring driver
//////////////////////////////////////////////////////////////////////////////
#ifndef __cfo_emu_frontend_hh__
#define __cfo_emu_frontend_hh__

#include "drivers/bus/null.h"

#include "utils/mu2e_sc.hh"

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
//-----------------------------------------------------------------------------
// emulated CFO : what equipment does it need ? - monitoring ! 
//-----------------------------------------------------------------------------
  {"EMU_CFO",                  // equipment name, to become CFO_EMU
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
   nullptr,                    // driver (driver list) : for emulated CFO, don't need drivers ..
   NULL,                       /* init string */
  },

  {""}
};

#endif
