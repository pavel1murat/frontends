///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "cfo_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "dtcInterfaceLib/DTC.h"
using namespace DTCLib; 

#include "frontends/utils/utils.hh"
#include "frontends/cfo_frontend/cfo_frontend.hh"
#include "frontends/cfo_frontend/cfo_driver.hh"
//-----------------------------------------------------------------------------
// Globals
// The frontend name (client name) as seen by other MIDAS clients
//-----------------------------------------------------------------------------
const char* frontend_name;

const char* frontend_file_name  = __FILE__; // The frontend file name, don't change it
BOOL        frontend_call_loop  =     TRUE; // frontend_loop is called periodically if this variable is TRUE
INT         display_period      =        0; // 1000; // if !=0, a frontend status page is displayed with this frequency in ms
INT         max_event_size      =    10000; // maximum event size produced by this frontend
INT         max_event_size_frag =  5*10000; // maximum event size for fragmented events (EQ_FRAGMENTED)
INT         event_buffer_size   = 10*10000; // buffer size to hold events */

//-----------------------------------------------------------------------------
// local scope variables
//-----------------------------------------------------------------------------
namespace {
  class FeName {
    std::string name;
  public:
    FeName() { 
      // name          = get_short_host_name("local");
      name         += "cfo_frontend";
      frontend_name = name.data();  // frontend_name is a global variable
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName xx;

  std::vector<DEVICE_DRIVER> _driver_list;


}


std::vector<DTCLib::DTC*> gDtc;

//-----------------------------------------------------------------------------
// CFO frontend init - needs a list of DTCs
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int         argc;
  char**      argv;
  char        active_run_conf[100];
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);
  int   sz = sizeof(active_run_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &active_run_conf, &sz, TID_STRING, FALSE);
//-----------------------------------------------------------------------------
// h_active_run_conf is the key corresponding the the active run configuration
//-----------------------------------------------------------------------------
  char     k_active_run_conf[200];
  HNDLE    h_active_run_conf;
  sprintf(k_active_run_conf,"/Mu2e/RunConfigurations/%s",active_run_conf);
	db_find_key(hDB, 0, k_active_run_conf, &h_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/DetectorConfigurations/$detector_conf/DAQ to get a list of
// nodes and DTC's to be monitored
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name("local");
  
  char  k_daq[200];
  HNDLE h_daq;
  sprintf(k_daq,"/Mu2e/RunConfigurations/%s/DetectorConfiguration/DAQ",active_run_conf);
	if (db_find_key(hDB, 0, k_daq, &h_daq) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "00121 no handle for:" << k_daq << ", got:" << h_daq;
  }
//-----------------------------------------------------------------------------
// loop over hosts in the configuration
//-----------------------------------------------------------------------------
  KEY   k_host_conf;
  HNDLE h_host_conf;
  for (int i=0; db_enum_key(hDB,h_daq,i,&h_host_conf) != DB_NO_MORE_SUBKEYS; i++) {
    db_get_key(hDB,h_host_conf,&k_host_conf);

    HNDLE h_subkey;
    KEY   k_subkey;
    for (int j=0; db_enum_key(hDB,h_host_conf,j,&h_subkey) != DB_NO_MORE_SUBKEYS; j++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
      db_get_key(hDB,h_subkey,&k_subkey);
      if (strstr(k_subkey.name,"DTC") != k_subkey.name)           continue;
//-----------------------------------------------------------------------------
// use only enabled DTCs
//-----------------------------------------------------------------------------
      int dtc_enabled(0);
      int sz = sizeof(INT);
      db_get_value(hDB, h_subkey, "Enabled", &dtc_enabled, &sz, TID_INT, FALSE);
      if (dtc_enabled == 1) {
//-----------------------------------------------------------------------------
// use the DTC - start from assuming that the DTC's are local
// to begin with, consider just one link - link0, expand later
//-----------------------------------------------------------------------------
        int link = 0;
        DTCLib::DTC* dtc = new DTC(DTCLib::DTC_SimMode_NoCFO,-1,0x1<<4*link,"");
        gDtc.push_back(dtc);
//-----------------------------------------------------------------------------
// for each DTC, define a driver
// so far, output of all drivers goes into the same common "Input" array
//-----------------------------------------------------------------------------
        DEVICE_DRIVER drv;

        snprintf(drv.name,NAME_LENGTH,"cfo_driver");
        drv.dd         = cfo_driver;    // cfo_driver;
        drv.channels   = 1;             // nwords in history - this one will be the number of events
        drv.bd         = null;
        drv.flags      = DF_INPUT;
        drv.enabled    = true;         // default true
        drv.dd_info    = nullptr;
        drv.mt_buffer  = nullptr;
        drv.pequipment = nullptr;
        
        _driver_list.push_back(drv);

      }
    }
  }

  _driver_list.push_back(DEVICE_DRIVER{""});
  equipment[0].driver = &_driver_list[0];


//   std::string k1 = key;
// //-----------------------------------------------------------------------------
// // DTC configuration on 'host_name'
// //-----------------------------------------------------------------------------
//   HNDLE h_daq_conf;
// 	if (db_find_key(hDB, 0, k1.data(), &h_daq_conf) != DB_SUCCESS) {
//     TLOG(TLVL_ERROR) << "0012 no handle for:" << k1 << ", got:" << h_daq_conf;
//   }
// //-----------------------------------------------------------------------------
// // DTC is the equipment, two are listed in the header, both should be listed in ODB
// //-----------------------------------------------------------------------------
//   for (int i=0; db_enum_key(hDB,h_daq_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//   }

  return CM_SUCCESS;
}

/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  printf("cfo_frontend: POLL_EVENT\n");
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
  ss_sleep(5000);
  TLOG(TLVL_DEBUG+10) << "frontend_loop ENTERED after ss_sleep 5000";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// at begin run want to clear all DTC counters
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  printf("cfo_frontend: BEGIN_RUN\n");
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
  printf("cfo_frontend: END_RUN\n");
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
