//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __dtc_frontend_hh__
#define __dtc_frontend_hh__

#include "drivers/class/hv.h"
#include "drivers/class/multi.h"
#include "drivers/bus/null.h"
#include "dtc_frontend/dtc_driver.hh"
/*-- Equipment list ------------------------------------------------*/

/* device driver list */

// DEVICE_DRIVER driver_list[] = {
//   {"dtc" , dtc_driver,  4, null, DF_INPUT},
//   {""}
// };

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

  {"DTC%02i",                  /* equipment name */
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
    "", "", ""} ,
   cd_multi_read,              /* readout routine */
   cd_multi,                   /* class driver main routine */
   //   driver_list,                /* device driver list */
   nullptr,                    // driver (driver list) initialized in frontend_init
   NULL,                       /* init string */
  },
  {""}
};

#endif
