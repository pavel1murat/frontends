//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __scfe_example_equipment_hh__
#define __scfe_example_equipment_hh__

#include "scfe_example/drv_example.hh"

/*-- Equipment list ------------------------------------------------*/

/* device driver list */
DEVICE_DRIVER hv_driver[] = {
   {"hv_drv_example", drv_example, 16, null},
   {""}
};

DEVICE_DRIVER multi_driver[] = {
   {"Input" , drv_example, 2, null, DF_INPUT },
   {"Output", drv_example, 2, null, DF_OUTPUT},
   {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {

   {"HV",                       /* equipment name */
    {3, 0,                      /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_SLOW,                   /* equipment type */
     0,                         /* event source */
     "FIXED",                   /* format */
     TRUE,                      /* enabled */
     // RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
     RO_ALWAYS,        /* read when running and on transitions */
     10000,                     /* read every 60 sec */
     0,                         /* stop run after this event limit */
     0,                         /* number of sub events */
     2000,                      /* log history at most every twoseconds */
     "", "", ""} ,
    cd_hv_read,                 /* readout routine */
    cd_hv,                      /* class driver main routine */
    hv_driver,                  /* device driver list */
    NULL,                       /* init string */
    },

   {"Environment",              /* equipment name */
    {4, 0,                      /* event ID, trigger mask */
     "SYSTEM",                  /* event buffer */
     EQ_SLOW,                   /* equipment type */
     0,                         /* event source */
     "FIXED",                   /* format */
     TRUE,                      /* enabled */
     // RO_RUNNING | RO_TRANSITIONS,        /* read when running and on transitions */
     RO_ALWAYS,        /* read when running and on transitions */
     // 10000,                     /* read every 10 sec */
     2000,                     /* read every 2 sec */
     0,                         /* stop run after this event limit */
     0,                         /* number of sub events */
     1,                         /* log history every event as often as it changes (max 1 Hz) */
     "", "", ""} ,
    cd_multi_read,              /* readout routine */
    cd_multi,                   /* class driver main routine */
    multi_driver,               /* device driver list */
    NULL,                       /* init string */
    },

   {""}
};

#endif
