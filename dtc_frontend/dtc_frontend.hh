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
DEVICE_DRIVER driver_list[] = {
  {"dtc_driver", dtc_driver,  4, null, DF_INPUT},
  {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

  {"DTC%02i",                  /* equipment name */
   {3, 0,                      /* event ID, trigger mask */
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   /* equipment type */
    0,                         /* event source */
    "FIXED",                   /* format */
    TRUE,                      /* enabled */
    RO_ALWAYS,                 /* read when running and on transitions */
    10000,                     /* read every 10 sec */
    0,                         /* stop run after this event limit */
    0,                         /* number of sub events */
    60,                        /* log history at most every 60 seconds */
    "", "", ""} ,
   cd_multi_read,              /* readout routine */
   cd_multi,                   /* class driver main routine */
   driver_list,                /* device driver list */
   NULL,                       /* init string */
  },
  {""}
};

#endif
