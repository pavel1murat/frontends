///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "dtc_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"

#include "dtc_frontend/dtc_frontend.hh"
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

//-----------------------------------------------------------------------------
// Frontend Init
// use 'channel' as the DTC ID
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int         argc;
  char**      argv;
  std::string active_run_conf;
  // char        key[1000];
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);

  odb_i->GetActiveRunConfig(hDB,active_run_conf);
  HNDLE h_active_run_conf = odb_i->GetRunConfigHandle(hDB,active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/DetectorConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name("local");

  HNDLE h_daq_host_conf = odb_i->GetDaqHostHandle(hDB,h_active_run_conf,host);
//-----------------------------------------------------------------------------
// DTC is the equipment, two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
  HNDLE h_subkey;
  KEY   subkey;
  for (int i=0; db_enum_key(hDB,h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
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
    _driver_list[1]     = {"",};

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

/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  return 1;
}

//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
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
