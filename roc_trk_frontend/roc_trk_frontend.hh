//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __roc_trk_frontend_hh__
#define __roc_trk_frontend_hh__

// #include "drivers/class/hv.h"
#include "drivers/class/multi.h"

#include "drivers/bus/null.h"
#include "frontends/roc_trk_frontend/roc_trk_driver.hh"

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER driver_list[] = {
  {"roc", driver ,  36, null, DF_INPUT},
  {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

  {"DTC01_ROC1",               /* equipment name */
   {11,                         // event ID
    0,                         // trigger mask
    "SYSTEM",                  /* event buffer */
    EQ_SLOW,                   // equipment type
    0,                         // event source
    "FIXED",                   /* format */
    TRUE,                      //
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
