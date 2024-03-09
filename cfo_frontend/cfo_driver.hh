///////////////////////////////////////////////////////////////////////////////
// Store any parameters the device driver needs in following 
//   structure. Edit the DTC_DRIVER_SETTINGS_STR accordingly. This 
//   contains usually the address of the device. For a CAMAC device
//   this could be crate and station for example.
///////////////////////////////////////////////////////////////////////////////

#include "dtcInterfaceLib/DTC.h"

typedef struct {
  int          address;
  int          dtcID;
  DTCLib::DTC* dtc;
} CFO_DRIVER_SETTINGS;

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


INT cfo_driver(INT cmd, ...);
