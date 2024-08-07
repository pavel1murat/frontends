//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#ifndef __tfm_launch_fend_hh_
#define __tfm_launch_fend_hh_

#include "drivers/device/nulldev.h"
#include "drivers/bus/null.h"
//#include "drivers/class/multi.h"
#include "utils/mu2e_sc.hh"
//-----------------------------------------------------------------------------
// for the TFM frontend, there is nothing to read, so the actual driver=nulldev
//-----------------------------------------------------------------------------
DEVICE_DRIVER driver_list[] = {
  {"tfm_launch_fend_driver", nulldev ,  1 , null, DF_INPUT},  // reserve the array larger than the immediate need
  {""}
};

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {
    "tfm_launch",                         // eq name
    EQUIPMENT_INFO {
      2, 0,                               // event ID, trigger mask
      "SYSTEM",                           // event buffer name
      // EQ_PERIODIC,                        // one of EQ_xx (equipment type)
      EQ_SLOW,                            // one of EQ_xx (equipment type)
      0,                                  // event source (LAM/IRQ)
      "FIXED",                            // data format to produce
      TRUE,                               // enable flag value
                                          // combination of Read-On flags R)_xxx - read when running and on transitions
                                          // and on ODB updates
      RO_RUNNING | RO_TRANSITIONS, // RO_ALWAYS,
      10000,                              // readout interval/polling time in ms (1 sec)
      0,                                  // stop run after this event limit - probably, 0=never
      0,                                  // number of sub events
      20,                                 // log history every 20 seconds
      "",                                 // host on which frontend is running
      "",                                 // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    // cd_multi_read,                        // pointer to user readout routine
    // cd_multi,                             // class driver routine
    cd_mu2e_sc_read,                        // pointer to user readout routine
    cd_mu2e_sc,                             // class driver routine
    driver_list,                          // device driver list
    nullptr,                              // init string for fixed events or bank list
    nullptr,                              // private data for class driver
    0,                                    // one of FE_xxx
    DWORD(0),                             // last time event was read
    DWORD(0),                             // last time idle func was called
    DWORD(0),                             // poll period
    0,                                    // FORMAT_xxx
    HNDLE(-1),                            // MIDAS buffer handle (int)
    HNDLE(-1),                           // handle to variable subtree in ODB
    DWORD(0),                            // serial number
    DWORD(0),                            // subevent number
    DWORD(0),                            // updates FE->ODB
    DWORD(0),                            // updates ODB-->FE
    DWORD(0),                            // bytes sent
    DWORD(0),                            // events sent
    DWORD(0),                            // events collected
    EQUIPMENT_STATS{
      0,                                 // events sent
      0,                                 // events_per_sec
      0                                  // kbytes_per_sec
    }
  },
//-----------------------------------------------------------------------------
// empty frontend structure marks the end of the equipment list
//-----------------------------------------------------------------------------
  {""}
};
#endif
