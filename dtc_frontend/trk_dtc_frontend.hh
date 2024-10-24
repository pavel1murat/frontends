//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __trk_dtc_flontend_hh__
#define __trk_dtc_flontend_hh__

#include "midas.h"

typedef int INT;

#include "drivers/bus/null.h"

#include "utils/mu2e_sc.hh"
//-----------------------------------------------------------------------------
// Equipment list
//-----------------------------------------------------------------------------

BOOL equipment_common_overwrite = TRUE;
//-----------------------------------------------------------------------------
// DTC frontend equipment list includes the following:
// - one or two DTC's
// - potentially, an "equipment" corresponding to a simulated CFO - need something to
//   be sending trains of pulses
// - an empty structure, marking the end
// start from a placeholder
//-----------------------------------------------------------------------------
EQUIPMENT equipment[4] = {

  {"${HOSTNAME}#DTC0",         // equipment name, to become mu2edaq09::DTC
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   /* equipment type */
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_ALWAYS,                 // read when running and on transitions - not really sure what this means
    // RO_TRANSITIONS, // | RO_RUNNING , // RO_ALWAYS,
    30000,                     // read every 30 sec - P.M. for EQ_SLOW, should this number be sync'ed with the log_history every number ?
    0,                         /* stop run after this event limit */
    0,                         /* number of sub events */
    30,                        // log history at most every 30 seconds - P.M. validated
    "",                        // host name
    "",                        // frontend name
    "",                        // frontend source file name
    "",                        // current status
    "",                        // color for status
    0,                         // hidden flag
    500000                     // write cache size
   } ,
   cd_mu2e_sc_read,              /* readout routine */
   cd_mu2e_sc,                   /* class driver main routine */
   //   driver_list,                /* device driver list */
   nullptr,                    // driver (driver list) initialized in frontend_init
   NULL,                       /* init string */
  },

  {"${HOSTNAME}#DTC1",         // equipment name, to become mu2edaq09::DTC
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   /* equipment type */
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_ALWAYS,                 // read when running and on transitions - not really sure what this means
    30000,                     // read every 30 sec - P.M. for EQ_SLOW, should this number be sync'ed with the log_history every number ?
    0,                         /* stop run after this event limit */
    0,                         /* number of sub events */
    30,                        // log history at most every 30 seconds - P.M. validated
    "",                        // host name
    "",                        // frontend name
    "",                        // frontend source file name
    "",                        // current status
    "",                        // color for status
    FALSE,                     // hidden flag
    500000                     // write cache size
   } ,
   cd_mu2e_sc_read,            /* readout routine */
   cd_mu2e_sc,                 /* class driver main routine */
   //   driver_list,           /* device driver list */
   nullptr,                    // driver (driver list) initialized in frontend_init
   NULL,                       /* init string */
  },

  {"${HOSTNAME}#emuCFO",
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   /* equipment type */
    0,                         /* event source */
    "FIXED",                   /* format */
    FALSE,                     // by default, no emulated CFO
    RO_ALWAYS,                 // read when running and on transitions - not really sure what this means
    1000,                      // "read" (call) once a sec - P.M. for EQ_SLOW ?
    0,                         /* stop run after this event limit */
    0,                         /* number of sub events */
    30,                        // log history at most every 30 seconds - P.M. validated
    "",                        // host name
    "",                        // frontend name
    "",                        // frontend source file name
    "",                        // current status
    "",                        // color for status
    FALSE,                     // hidden flag
    500000                     // write cache size
   } ,
   cd_mu2e_sc_read,            /* readout routine */
   cd_mu2e_sc,                 /* class driver main routine */
   //   driver_list,           /* device driver list */
   nullptr,                    // driver (driver list) initialized in frontend_init
   NULL,                       /* init string */
  },

  {"",}
};

#endif
