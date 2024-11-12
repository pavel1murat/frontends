//
#ifndef __dtc_driver_types__
#define __dtc_driver_types__

// #include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

typedef struct {
  int                   pcieAddr;
  int                   enabled;
  int                   ewLength;       // emulated CFO only
  uint64_t              nEvents;        // emulated CFO only
  uint64_t              firstEWTag;     // emulated CFO only
} DTC_DRIVER_SETTINGS;

#define DTC_DRIVER_SETTINGS_STR "\
PcieAddr   = INT    : %i\n\
Enabled    = INT    : %i\n\
EWLength   = UINT64 : 0\n\
NEvents    = UINT64 : 0\n\
FirstEWTag = UINT64 : 0\n\
"
/* the following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  DTC_DRIVER_SETTINGS driver_settings;
  float               *array;
  INT                 num_channels;
  INT                 (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void                *bd_info;                /* private info of bus driver  */
  HNDLE               hkey;                    /* ODB key for bus driver info */
} DTC_DRIVER_INFO;

#endif
