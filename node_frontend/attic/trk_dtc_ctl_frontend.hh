//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __trk_dtc_ctl_flontend_hh__
#define __trk_dtc_ctl_flontend_hh__

#include "midas.h"

typedef int INT;

// #include "drivers/bus/null.h"
// #include "utils/mu2e_sc.hh"
//-----------------------------------------------------------------------------
// Equipment list
//-----------------------------------------------------------------------------

BOOL equipment_common_overwrite = TRUE;
//-----------------------------------------------------------------------------
// DTC control frontend manages DTCs on a given node
//-----------------------------------------------------------------------------
EQUIPMENT equipment[2];

EQUIPMENT eqq[3] = {

  {"${HOSTNAME}#DTC0-ctl",     // equipment name, to become mu2edaq09#DTC0-ctl
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_USER,                   // equipment type, for DTC-ctl doesn't need to be read out at all
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_ALWAYS,                 // read when running and on transitions - not really sure what this means
    // RO_TRANSITIONS, // | RO_RUNNING , // RO_ALWAYS,
    30000,                     // read every 30 sec
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
   nullptr,                    /* readout routine */
   nullptr,                    /* class driver main routine */
   nullptr,                    // no drivers
   NULL,                       /* init string */
  },

  {"${HOSTNAME}#DTC1-ctl",     // equipment name, to become mu2edaq09#DTC-ctl
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_USER,                   /* equipment type */
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_ALWAYS,                 // read when running and on transitions - not really sure what this means
    30000,                     // read every 30 sec
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
   nullptr,                    /* readout routine */
   nullptr,                    /* class driver main routine */
   nullptr,                    // no drivers for DTC-ctl
   NULL,                       /* init string */
  },

  {"",}
};

#endif
