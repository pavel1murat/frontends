///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.h by Stefan Ritt
// ARTDAQ data receiver slow control driver
///////////////////////////////////////////////////////////////////////////////
/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

#ifndef __tfm_disk_driver_hh__
#define __tfm_disk_driver_hh__

#include "midas.h"

typedef struct {
  int  Link;
  int  Active;
  char CompName[NAME_LENGTH];
  char HostName[NAME_LENGTH];
} TFM_DISK_DRIVER_SETTINGS;

/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  TFM_DISK_DRIVER_SETTINGS  driver_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} TFM_DISK_DRIVER_INFO;

INT tfm_disk_driver(INT cmd, ...);
//-----------------------------------------------------------------------------
// ARTDAQ metrics reported by the data receiver, assume everything is 4 bytes
//-----------------------------------------------------------------------------
struct DiskDriverData_t {
                                      // line 0 doesn't have any numbers in it

  float  fileSize;                    // [ 0] output file size, MB
  int    ioRate;                      // [ 1] instantaneous data rate, MB/sec
};
//-----------------------------------------------------------------------------
// internally, a MIDAS driver stored only floats
//-----------------------------------------------------------------------------
int const TFM_DISK_DRIVER_NWORDS =  10;

#endif
