#ifndef __trk_spi_data_hh__
#define __trk_spi_data_hh__

#include "midas.h"

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {
    "DTC1",                               // eq name
    EQUIPMENT_INFO {
      2, 0,                               // event ID, trigger mask
      "SYSTEM",                           // event buffer name
      EQ_PERIODIC,                        // one of EQ_xx (equipment type)
      0,                                  // event source (LAM/IRQ)
      "MIDAS",                            // data format to produce
      TRUE,                               // enable flag value
                                          // combination of Read-On flags R)_xxx - read when running and on transitions 
                                          // and on ODB updates
      RO_RUNNING | RO_TRANSITIONS | RO_ODB,
      1000,                               // readout interval/polling time in ms (1 sec)
      0,                                  // stop run after this event limit - probably, 0=never
      0,                                  // number of sub events
      10,                                 // log history every ten seconds
      "",                                 // host on which frontend is running
      "",                                 // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    read_periodic_event,                  // pointer to user readout routine
    nullptr,                              // class driver routine
    nullptr,                              // device driver list
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
// an empty frontend structure marks end of the equipment list
//-----------------------------------------------------------------------------
  {"",                                   // empty structure
   EQUIPMENT_INFO {
      2, 0,                               // event ID, trigger mask
      "SYSTEM",                           // event buffer name
      EQ_PERIODIC,                        // one of EQ_xx (equipment type)
      0,                                  // event source (LAM/IRQ)
      "MIDAS",                            // data format to produce
      TRUE,                               // enable flag value
      // combination of Read-On flags R)_xxx - read when running and on transitions 
      // and on ODB updates
      RO_RUNNING | RO_TRANSITIONS | RO_ODB,
      1000,                               // readout interval/polling time in ms (1 sec)
      0,                                  // stop run after this event limit - probably, 0=never
      0,                                  // number of sub events
      10,                                 // log history every ten seconds
      "",                                 // host on which frontend is running
      "",                                 // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    read_periodic_event,                  // pointer to user readout routine
    nullptr,                              // class driver routine
    nullptr,                              // device driver list
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
  }
};

#endif
