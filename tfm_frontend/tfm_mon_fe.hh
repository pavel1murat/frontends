//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
#ifndef __tfm_mon_fe_hh_
#define __tfm_mon_fe_hh_

#include "drivers/bus/null.h"
#include "frontends/utils/mu2e_sc.hh"
#include "frontends/tfm_frontend/tfm_br_driver.hh"
#include "frontends/tfm_frontend/tfm_dr_driver.hh"
#include "frontends/tfm_frontend/tfm_disk_driver.hh"
//-----------------------------------------------------------------------------
/* device driver list */
//-----------------------------------------------------------------------------
// it is possible that having separate drivers for a boardreader, datareceiver and a file
// is a good idea, "multi" would by a natural choice
// but how would that play with the total number of threads ?
// in both cases, reserve array sizes > than immediate needs for driver internal buffers 
// *_NWORDS are defined in the corresponding include files respectively
//-----------------------------------------------------------------------------
// DEVICE_DRIVER driver_list[] = {
//   {"tfm_driver"   , tfm_driver   , TFM_DRIVER_NWORDS   , null, DF_INPUT},
//   {"tfm_br_driver", tfm_br_driver, TFM_BR_DRIVER_NWORDS, null, DF_INPUT},
//   {""}
// };

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {
    "${HOSTNAME}#artdaq",                 // eq name
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
      RO_ALWAYS, // RO_RUNNING | RO_TRANSITIONS, // RO_ALWAYS,
      10000,                              // readout interval/polling time in ms (10 sec)
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
    cd_mu2e_sc_read,                      // pointer to user readout routine, for monitoring-only client could be a nullptr
    cd_mu2e_sc     ,                      // class driver routine
    //    driver_list,                          // device driver list
    nullptr,                              // device driver list, initialized dynamically in frontend_init (P.M.)
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
