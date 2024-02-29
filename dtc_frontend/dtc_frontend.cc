/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Example Slow Control Frontend program. Defines two
                slow control equipments, one for a HV device and one
                for a multimeter (usually a general purpose PC plug-in
                card with A/D inputs/outputs. As a device driver,
                the "null" driver is used which simulates a device
                without accessing any hardware. The used class drivers
                cd_hv and cd_multi act as a link between the ODB and
                the equipment and contain some functionality like
                ramping etc. To form a fully functional frontend,
                the device driver "null" has to be replaces with
                real device drivers.

  $Id$

\********************************************************************/
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "dtc_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "frontends/utils/utils.hh"
#include "frontends/dtc_frontend/dtc_frontend.hh"
//-----------------------------------------------------------------------------
// Globals
// The frontend name (client name) as seen by other MIDAS clients
//-----------------------------------------------------------------------------
const char* frontend_name;

namespace {
  class FeName {
    std::string name;
  public:
    FeName() { 
      name = get_short_host_name("local");
      name += "_dtc";
      frontend_name = name.data();
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName xx;
}

static DEVICE_DRIVER* _driver_list (nullptr);


const char *frontend_file_name = __FILE__; // The frontend file name, don't change it
BOOL frontend_call_loop        = TRUE;     // frontend_loop is called periodically if this variable is TRUE
INT display_period             = 1000;     // a frontend status page is displayed with this frequency in ms
INT max_event_size             = 10000;    // maximum event size produced by this frontend
INT max_event_size_frag        = 5*10000;  // maximum event size for fragmented events (EQ_FRAGMENTED)
INT event_buffer_size          = 10*10000; // buffer size to hold events */

/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  return 1;
}

//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
}

//-----------------------------------------------------------------------------
// Frontend Init
// use 'channel' as the DTC ID
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int         argc;
  char**      argv;
  char        active_run_conf[100];
  char        key[1000];
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);
  int   sz = sizeof(active_run_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &active_run_conf, &sz, TID_STRING, TRUE);
//-----------------------------------------------------------------------------
// h_active_run_conf is the key corresponding the the active run configuration
//-----------------------------------------------------------------------------
  HNDLE    h_active_run_conf;
  sprintf(key,"/Mu2e/RunConfigurations/%s",active_run_conf);
	db_find_key(hDB, 0, key, &h_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/DetectorConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name(host_name);
  sprintf(key,"/Mu2e/RunConfigurations/%s/DetectorConfiguration/DAQ/%s",active_run_conf,host.data());
  std::string k1 = key;
                                        // DTC configuration on 'host_name'
  HNDLE h_daq_conf;
	if (db_find_key(hDB, 0, k1.data(), &h_daq_conf) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "0012 no handle for:" << k1 << ", got:" << h_daq_conf;
  }
//-----------------------------------------------------------------------------
// DTC is the equipment, two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
  HNDLE h_subkey;
  KEY   subkey;
  for (int i=0; db_enum_key(hDB,h_daq_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);
    if (strstr(subkey.name,"DTC") != subkey.name)           continue;
//-----------------------------------------------------------------------------
// for each DTC, define a driver
// so far, output of all drivers goes into the same common "Input" array
//-----------------------------------------------------------------------------
    _driver_list        = new DEVICE_DRIVER[2];
    _driver_list[1]     = {""};

    DEVICE_DRIVER* drv = &_driver_list[0];
    
    snprintf(drv->name,NAME_LENGTH,"dtc%i",i);
    drv->dd         = dtc_driver;
    drv->channels   = DTC_NREG_HIST;    // nwords recorded as history (4)
    drv->bd         = null;
    drv->flags      = DF_INPUT;
    drv->enabled    = true;
    drv->dd_info    = nullptr;
    drv->mt_buffer  = nullptr;
    drv->pequipment = nullptr;

    equipment[i].driver = _driver_list;
  }


  return CM_SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit() {
  return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop() {
  ss_sleep(5);
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// at begin run want to clear all DTC counters
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
   return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error) {
   return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error) {
   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
