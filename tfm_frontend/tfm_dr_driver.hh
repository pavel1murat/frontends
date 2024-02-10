///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.h by Stefan Ritt
// ARTDAQ data receiver slow control driver
///////////////////////////////////////////////////////////////////////////////
/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

#ifndef __tfm_dr_driver_hh__
#define __tfm_dr_driver_hh__

#include "midas.h"

typedef struct {
  int  Link;
  int  Active;
  char CompName [NAME_LENGTH];
  char XmlrpcUrl[64];
} TFM_DR_DRIVER_SETTINGS;

/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  TFM_DR_DRIVER_SETTINGS  driver_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} TFM_DR_DRIVER_INFO;

INT tfm_dr_driver(INT cmd, ...);
//-----------------------------------------------------------------------------
// ARTDAQ metrics reported by the data receiver, assume everything is 4 bytes
//-----------------------------------------------------------------------------
struct ReceiverStatData_t {
                                      // line 0 doesn't have any numbers in it

  int    nEventsRead;                 // [ 0] n(events) within the time window
  int    eventRate;                   // [ 1] end of line
  float  dataRateEv;                  // [ 2] MB/sec
  float  timeWindow;                  // [ 3]
  float  minEventSize;                // [ 4] MB
  float  maxEventSize;                // [ 5] MB , end of line 1
  float  elapsedTime;                 // [ 6]      end of line 2

  int    nFragRead;                   // [ 7] N(fragments) within the time window
  float  fragRate;                    // [ 8] 
  float  dataRateFrag;                // [ 9] MB/sec
  int    minFragSize;                 // [10] per getNext call
  int    maxFragSize;                 // [11] per getNext call , end of line 3

  int    nEvTotRun;                   // [12]
  int    nEvBadRun;                   // [13]
  int    nEvTotSubrun;                // [14]
  int    nEvBadSubrun;                // [15] end of line 4

  int    nShmBufTot;                  // [16] total number of allocated SHM buffers
  int    nShmBufEmpty;                // [17] n empty (free) buffers
  int    nShmBufWrite;                // [18] N buffers being written to 
  int    nShmBufFull;                 // [19] N full buffers 
  int    nShmBufRead;                 // [20] N buffers being read from
};
//-----------------------------------------------------------------------------
// internally, a MIDAS driver stored only floats
//-----------------------------------------------------------------------------
int const TFM_DR_DRIVER_NWORDS =  30;

#endif
