/********************************************************************\

  Name:         nulldev.h
  Created by:   Stefan Ritt

  Contents:     Device driver function declarations for NULL device

  $Id:$

\********************************************************************/
/* Store any parameters the device driver needs in following 
   structure. Edit the DTC_DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

#include "dtc_frontend/dtc_driver_types.hh"

INT emucfo_driver(INT cmd, ...);
