#ifndef __cfo_frontend_cfo_interface_hh__
#define __cfo_frontend_cfo_interface_hh__

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"

typedef struct {
  int           pcieAddr;
  int           external;      // 0: "emulated": 1: "external"
                               // 1: "external": real CFO, 'interface' is of CfoInterface type
  void*         interface;     // pointer (CfoInterface* or DtcInterface*, see above)
                               // before use, 'interface' need to be cast
  int           n_ewm_per_sec; // N(EWMs) per second
} CFO_DRIVER_SETTINGS;

#define CFO_DRIVER_SETTINGS_STR "\
PcieAddr       = INT        : %i\n\
External       = INT        : %i\n\
Cfo            = INT64      :  0\n\
NEwmsPerSecond = INT        : %i\n\
"

/* the following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  CFO_DRIVER_SETTINGS driver_settings;
  float               *array;
  INT                 num_channels;
  INT                 (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void                *bd_info;                /* private info of bus driver  */
  HNDLE               hkey;                    /* ODB key for bus driver info */
} CFO_DRIVER_INFO;

#endif
