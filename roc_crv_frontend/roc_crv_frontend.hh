//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __roc_frontend_hh__
#define __roc_frontend_hh__

// #include "drivers/class/hv.h"
//#include "drivers/class/multi.h"
#include "roc_crv_frontend/cd_multi.h"

#include "drivers/bus/null.h"
#include "roc_crv_frontend/roc_crv_driver.hh"

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER driver_list[] = {
  {"rocOut", roc_crv_driver ,  25, null, DF_INPUT},
  {"rocBias",roc_crv_driver ,  16, null, DF_OUTPUT},
  {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

  {"ROC1_CRV",               /* equipment name */
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
