///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.h by Stefan Ritt
// artdaq boardreader slow control driver
///////////////////////////////////////////////////////////////////////////////
#ifndef __tfm_br_driver_hh__
#define __tfm_br_driver_hh__
/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */
//-----------------------------------------------------------------------------
// a boardreader can read several fragment types, the number of lines 
// in the shared memory manager report depends on that
//-----------------------------------------------------------------------------
typedef struct {
  int  Link;
  int  Active;
  int  NFragmentTypes;
  char CompName [NAME_LENGTH];
  char XmlrpcUrl[64];
} TFM_BR_DRIVER_SETTINGS;


/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  TFM_BR_DRIVER_SETTINGS  driver_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} TFM_BR_DRIVER_INFO;

INT tfm_br_driver(INT cmd, ...);

//-----------------------------------------------------------------------------
// metrics reported by the ARTDAQ boardreader
//-----------------------------------------------------------------------------
struct BrStatData_t {
  int    runNumber;                     // [ 0]
  int    nFragTot;                      // [ 1] end of line 0
  int    nFragRead;                     // [ 2]
  float  getNextRate;                   // [ 3] per second;
  float  fragRate;                      // [ 4]
  float  timeWindow;                    // [ 5]
  int    minNFrag;                      // [ 6] per getNext call
  int    maxNFrag;                      // [ 7] per getNext call
  float  elapsedTime;                   // [ 8] 
  float  dataRate;                      // [ 9] MB/sec
  float  minEventSize;                  // [10] MB
  float  maxEventSize;                  // [11] MB , end of line 1
  float  inputWaitTime;                 // [12]
  float  bufferWaitTime;                // [13]
  float  requestWaitTime;               // [14]
  float  outputWaitTime;                // [15] per fragment; end of line 2
  int    fragID  [5];                   // [16-20] fragment IDs
  int    nShmFragsID[5];                // [21-25] n(fragments) with fragID[i] currently in the SHM buffer
  int    nShmBytesID[5];                // [26-30] n(bytes)     for  fragID[i] currently in the SHM buffer
};

int const TFM_BR_DRIVER_NWORDS =  40;

#endif
