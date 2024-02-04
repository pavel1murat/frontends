///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.h by Stefan Ritt
// ARTDAQ data receiver slow control driver
///////////////////////////////////////////////////////////////////////////////
/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

#include "midas.h"

typedef struct {
  int  Link;
  int  Active;
  char NodeName[NAME_LENGTH];
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
  int    nEventsRead;                 // n(events) within the time window
  int    eventRate;                   // end of line
  float  dataRateEv;                  // MB/sec
  float  timeWindow;
  float  minEventSize;                // MB
  float  maxEventSize;                // MB , end of line 1
  float  elapsedTime;                 //      end of line 2

  int    nFragRead;                   // N(fragments) within the time window
  float  fragRate;
  float  dataRateFrag;                // MB/sec
  int    minFragSize;                 // per getNext call
  int    maxFragSize;                 // per getNext call , end of line 3

  int    nEvTotRun;                   // 
  int    nEvBadRun;                   // 
  int    nEvTotSubrun;                // 
  int    nEvBadSubrun;                // end of line 4

  int    nShmBufTot;                  // total number of allocated SHM buffers
  int    nShmBufEmpty;                // n empty (free) buffers
  int    nShmBufWrite;                // N buffers being written to 
  int    nShmBufFull;                 // N full buffers 
  int    nShmBufRead;                 // N buffers being read from
};
//-----------------------------------------------------------------------------
// internally, a MIDAS driver stored only floats
//-----------------------------------------------------------------------------
int const TFM_DR_DRIVER_NWORDS =  30;

