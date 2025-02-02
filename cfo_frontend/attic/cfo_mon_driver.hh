///////////////////////////////////////////////////////////////////////////////
// Store any parameters the device driver needs in following 
//   structure. Edit the DTC_DRIVER_SETTINGS_STR accordingly. This 
//   contains usually the address of the device. For a CAMAC device
//   this could be crate and station for example.
///////////////////////////////////////////////////////////////////////////////
#ifndef __cfo_mon_driver_hh__
#define __cfo_mon_driver_hh__

#include "cfo_interface.hh"

INT cfo_mon_driver(INT cmd, ...);


#endif
