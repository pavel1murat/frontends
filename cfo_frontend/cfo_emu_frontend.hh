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

int cfo_emu_launch_run_plan(char *pevent, int);

EQUIPMENT equipment[] = {
//-----------------------------------------------------------------------------
// emulated CFO : what equipment does it need ? - monitoring ! 
//-----------------------------------------------------------------------------
  {"EmulatedCFO",              // equipment name
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_PERIODIC,               // equipment type. PM: emulated CFO better be periodic !
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_RUNNING,                // RO_TRANSITIONS, // | RO_RUNNING , // RO_ALWAYS,
    500,                       // period, 0.5 msec; if 0 do not do anything .. 10000 used by EQ_PERIODIC
                               // P.M. for EQ_SLOW, should this number be sync'ed with 
                               // the log_history every number ?
    0,                         /* event_limit */
    0,                         /* number of sub events */
    500,                       // [ms] 'history' log history at most every 30 seconds - P.M. validated
    "",                        // host name
    "",                        // frontend name
    "",                        // frontend source file name
    "",                        // current status
    "",                        // color for status
    0,                         // hidden flag
    0                          // write cache size
   } ,
   cfo_emu_launch_run_plan,    // readout routine, called periodically - this is it !
   cd_mu2e_sc,                 /* class driver main routine */
   //   driver_list,           // device driver list
   nullptr,                    // driver (driver list) : for emulated CFO, don't need drivers ..
   NULL,                       /* init string */
   nullptr,                    // class driver info ????
  },

  {"",}
};

#endif
