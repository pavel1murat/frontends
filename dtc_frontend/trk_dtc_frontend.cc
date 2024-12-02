///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
// the DTC frontend is also responsible for properly initializing the DTCs on a given host
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "trk_dtc_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
using namespace DTCLib;
using namespace trkdaq;

#include "dtc_frontend/trk_dtc_driver.hh"
#include "dtc_frontend/trk_dtc_frontend.hh"
//-----------------------------------------------------------------------------
// Globals
// The frontend name (client name) as seen by other MIDAS clients
//-----------------------------------------------------------------------------
const char* frontend_name;

namespace {
  class FeName {
  public:
    std::string name;

    FeName() { 
      name = get_short_host_name("local");
      name += "_dtc";
      frontend_name = name.data();
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName        _xx;

  std::string   _active_run_conf;
  HNDLE         _h_active_run_conf(0);
  HNDLE         _h_host_conf(0);
  int           _ndtcs(0);                       // 0,1, or 2
  std::string   _rpc_host;
//-----------------------------------------------------------------------------  
// to be passed to the callback
//-----------------------------------------------------------------------------  
  struct DtcData_t {
    DtcInterface* dtc_i;
    HNDLE         h_dtc;
  } _dtc_data[2];

}

const char *frontend_file_name  = __FILE__; // The frontend file name, don't change it

BOOL frontend_call_loop         = FALSE;    // TRUE; // frontend_loop is called periodically if this variable is TRUE
INT  display_period             = 1000;     // a frontend status page is displayed with this frequency in ms
INT  max_event_size             = 10000;    // maximum event size produced by this frontend
INT  max_event_size_frag        = 5*10000;  // maximum event size for fragmented events (EQ_FRAGMENTED)
INT  event_buffer_size          = 10*10000; // buffer size to hold events */
//-----------------------------------------------------------------------------
// callbacks
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error);

//-----------------------------------------------------------------------------
// command can only be issued is status = 0 (?)
// so far, has only one command - configure
//-----------------------------------------------------------------------------
void process_command(INT hDB, INT hkey, int A, void* Info) {
  HNDLE h_cmd;
  KEY   key;
  db_get_key(hDB,hkey,&key);

  DtcData_t* dd = (DtcData_t*) Info;
  trkdaq::DtcInterface* dtc_i = dd->dtc_i;
  int pcie_addr = dtc_i->fPcieAddr;

  TLOG(TLVL_DEBUG) << key.name << "DTC PCIE ADDR:" << pcie_addr;

  char cmd_path[100];
  sprintf(cmd_path,"/Mu2e/Commands/Configure/DAQ/%s/DTC%i",_rpc_host.data(),pcie_addr);

  db_find_key(hDB, 0, cmd_path, &h_cmd);

  char params[1000];
  int  run, status;
  int  done(0);
  int  sz_int = sizeof(int);
  int  sz     = sizeof(params);
  db_get_value(hDB, h_cmd, "Run"       , &run   , &sz_int, TID_INT32 , false);
  db_get_value(hDB, h_cmd, "Parameters", params , &sz    , TID_STRING, false);
  db_get_value(hDB, h_cmd, "Status"    , &status, &sz_int, TID_INT32 , false);
  
  TLOG(TLVL_DEBUG) << "Process Command:" << key.name << "Value:" << run
                   << " Parameters: "    << params   << " Status:" << status;
  if (run == 0) return;
//-----------------------------------------------------------------------------
// mark command as being executed
//-----------------------------------------------------------------------------
  int EXECUTION_IN_PROGRESS(1);
  db_set_value(hDB,h_cmd,"Status",&EXECUTION_IN_PROGRESS,sizeof(int),1,TID_INT32);
  
  if (run == 1) {
                                        // run = 1 : configure the DTCs
    char key[200];
    if (dtc_i->Enabled()) {
      status = dtc_i->InitReadout();
      TLOG(TLVL_DEBUG) << " init DTC readout" ;
    }
    else {
      TLOG(TLVL_WARNING) << " DTC " << pcie_addr << " is not enabled";
    }
//-----------------------------------------------------------------------------
// set DTCstatus in ODB and mark the command as executed
//-----------------------------------------------------------------------------
    db_set_value(hDB,h_cmd,"Status",&status,sizeof(int),1,TID_INT32);
    db_set_value(hDB,h_cmd,"Run"   ,&done  ,sizeof(int),1,TID_INT32);
  };

  TLOG(TLVL_DEBUG) << " finished" ;
}

//-----------------------------------------------------------------------------
// Frontend Init
// use 'channel' as the DTC ID
//----------------------------------------------------------------------------- 
INT frontend_init() {
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  int         argc;
  char**      argv;
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);
  _active_run_conf    = odb_i->GetActiveRunConfig(hDB);
  _h_active_run_conf  = odb_i->GetRunConfigHandle(hDB,_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _rpc_host  = get_short_host_name("local");

  TLOG(TLVL_DEBUG+2) << "rpc_host:" << _rpc_host;

  _h_host_conf = odb_i->GetDaqHostHandle(hDB,_h_active_run_conf,_rpc_host);
//-----------------------------------------------------------------------------
// DTC is the equipment, two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
  HNDLE h_subkey;
  KEY   subkey;

  //  int      cfo_pcie_addr(-1);
  //  uint64_t nevents      (0);
  //  int      ew_length    (0);
  //  uint64_t first_ew_tag (0);

  for (int i=0; db_enum_key(hDB,_h_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);

    TLOG(TLVL_DEBUG+2) << "subkey.name:" << subkey.name;

    if (strstr(subkey.name,"DTC") != subkey.name)           continue;

    int enabled        = odb_i->GetDtcEnabled    (hDB,h_subkey);
    int pcie_addr      = odb_i->GetPcieAddress   (hDB,h_subkey);
    
    memcpy(&equipment[_ndtcs],&eqq[pcie_addr],sizeof(EQUIPMENT));
    EQUIPMENT* eqp     = &equipment[_ndtcs];
    sprintf(eqp->name,"%s#DTC%i",_rpc_host.data(),pcie_addr);
    eqp->info.enabled  = (enabled == 1);

    TLOG(TLVL_DEBUG+2) << "DTC enabled today:" << enabled << " pcie_addr:" << pcie_addr;
//-----------------------------------------------------------------------------
// the link mask will be defined by looping over the panels
//-----------------------------------------------------------------------------
    int link_mask      = odb_i->GetDtcLinkMask   (hDB,h_subkey);
    TLOG(TLVL_DEBUG+2) << "link_mask:0x" <<std::hex << link_mask << std::endl; 
//-----------------------------------------------------------------------------
// this is the only place where the DTC is fully initialized
// everywhere else should use skip_init = true
//-----------------------------------------------------------------------------
    bool skip_init          = false;
    DtcInterface* dtc_i     = DtcInterface::Instance(pcie_addr,link_mask,skip_init);
    _dtc_data[_ndtcs].dtc_i = dtc_i;
    _dtc_data[_ndtcs].h_dtc = h_subkey;
      
    dtc_i->fEnabled        = enabled;
    dtc_i->fRocReadoutMode = odb_i->GetDtcReadoutMode   (hDB,h_subkey);
    dtc_i->fSampleEdgeMode = odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
    dtc_i->fEmulateCfo     = odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);

    TLOG(TLVL_DEBUG+2) << "readout_mode:"      << dtc_i->fRocReadoutMode
                       << " sample_edge_mode:" << dtc_i->fSampleEdgeMode
                       << " emulate_cfo:"      << dtc_i->fEmulateCfo;
//     if (enabled) {
//       if (dtc_i->EmulateCfo()) {
//         if (cfo_pcie_addr >= 0) {
//           TLOG(TLVL_ERROR) << "redefinition of the emulated CFO" << std::endl;
//         }
//         else {
//           cfo_pcie_addr = pcie_addr;
//         }
//       }
// //-----------------------------------------------------------------------------
// // there can be only one CFO, so don't worry about duplication
// // enable emulated CFO, read parameters 
// //-----------------------------------------------------------------------------
//       nevents                  = odb_i->GetNEvents   (hDB,h_subkey);
//       ew_length                = odb_i->GetEWLength  (hDB,h_subkey);
//       first_ew_tag             = odb_i->GetFirstEWTag(hDB,h_subkey);
//     }
//-----------------------------------------------------------------------------
// for each DTC, define a driver
// so far, output of all drivers goes into the same common "Input" array
// if a DTC runs in an emulated mode: define one more piece of equipment with a driver
//-----------------------------------------------------------------------------
    DEVICE_DRIVER* drv_list = new DEVICE_DRIVER[2];
    DEVICE_DRIVER* drv      = &drv_list[0];
    
    snprintf(drv->name,NAME_LENGTH,"dtc%i",pcie_addr);
    drv->dd         = null; // FIXME - DEBUG trk_dtc_driver;
    drv->channels   = DTC_NWORDS_HIST;  // nwords recorded as history (4+6*36 = 220)
    drv->bd         = null;
    drv->flags      = DF_INPUT;
    drv->enabled    = enabled;

    DTC_DRIVER_INFO* ddi          = new DTC_DRIVER_INFO;
    ddi->driver_settings.pcieAddr = pcie_addr;
    ddi->driver_settings.enabled  = enabled;
    drv->dd_info                  = (void*) ddi;
      
    drv->mt_buffer  = nullptr;
    drv->pequipment = nullptr;

    drv_list[1]     = {"",};
    eqp->driver     = drv_list;
//-----------------------------------------------------------------------------
// finally, register the configuration callback
//-----------------------------------------------------------------------------
    char cmd_path[100];
    sprintf(cmd_path,"/Mu2e/Commands/Configure/DAQ/%s/DTC%i/Run",_rpc_host.data(),pcie_addr);

    HNDLE hkey;
    db_find_key(hDB, 0,cmd_path, &hkey);
    TLOG(TLVL_DEBUG) << "cmd_path: " << cmd_path << " hkey:  " << hkey ;

    DtcData_t* info = &_dtc_data[pcie_addr];
    if (db_watch(hDB, hkey, process_command, info) != DB_SUCCESS) {
      char msg[200];
      sprintf(msg,"Cannot connect to %s in ODB",cmd_path);
      cm_msg(MERROR, "frontend_init",msg);
      return -1;
    }

    TLOG(TLVL_DEBUG) << "watching " << cmd_path << " . --- DONE";

    _ndtcs         += 1;
  }
  // last name - empty
  equipment[_ndtcs].name[0] = 0;

//   if (cfo_pcie_addr >= 0) {
// //-----------------------------------------------------------------------------
// // after the DTCs are processed, add equipment called CFO with one driver
// //-----------------------------------------------------------------------------
//     EQUIPMENT* eqp = &equipment[2];
    
//     eqp->info.enabled  = 1;
      
//     DEVICE_DRIVER* drv_list  = new DEVICE_DRIVER[2];
//     DEVICE_DRIVER* drv       = &drv_list[0];
    
//     snprintf(drv->name,NAME_LENGTH,"emu_cfo");
//     drv->dd         = emucfo_driver;
//     drv->channels   = 1;                // fake, CFO driver is only needed to send event window markers
//     drv->bd         = null;
//     drv->flags      = DF_INPUT;
//     drv->enabled    = true;

//     DTC_DRIVER_INFO* ddi            = new DTC_DRIVER_INFO;
//     ddi->driver_settings.pcieAddr   = cfo_pcie_addr;
//     ddi->driver_settings.enabled    = 1;
//     ddi->driver_settings.ewLength   = ew_length;
//     ddi->driver_settings.nEvents    = nevents;
//     ddi->driver_settings.firstEWTag = first_ew_tag;
//     drv->dd_info                    = (void*) ddi;
    
//     drv->mt_buffer  = nullptr;
//     drv->pequipment = nullptr;

//     drv_list[1]     = {"",};
//     eqp->driver     = drv_list;
//   }
//-----------------------------------------------------------------------------
// transitions
//-----------------------------------------------------------------------------
//  cm_register_transition(TR_START,tr_prestart,500);

  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// callback
// the DTC frontend is doing all configurations for the DTC-only mode, as well as
// when running in the emulated CFO mode
// in this mode th DTC fontend also sends the pulses in a loop function
//-----------------------------------------------------------------------------
// INT process_command(INT run_number, char *error)  {
//   // code to perform actions prior to frontend starting 

//   TLOG(TLVL_DEBUG+2) << "got _run_configure : " << _run_configure;
  
//   // for (int i=0; i<_ndtcs; i++) {
//   //   trkdaq::DtcInterface* dtc_i = _dtc_i[i];
//   //   if (dtc_i->Enabled()) {
//   //     // initialize only enabled DTCs
//   //     dtc_i->InitReadout(dtc_i->EmulateCfo(),dtc_i->fReadoutMode);
//   //   }
//   // }
// //-----------------------------------------------------------------------------
// // after the command is executed, need to set status and 
// //-----------------------------------------------------------------------------
//   HNDLE h_command;
//   db_find_key(hDB, 0, "/Mu2e/Commands/Tracker/Configure", &h_command);
//   int run    = 0;
//   int status = 0;
  
//   db_set_value(hDB, h_command, "Run"   , &run   , sizeof(int), 1, TID_INT32);
//   db_set_value(hDB, h_command, "Status", &status, sizeof(int), 1, TID_INT32);
  
//   TLOG(TLVL_DEBUG+3) << "--- process_command DONE";
//   return CM_SUCCESS;  
// }


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
  TLOG(TLVL_DEBUG+2) << " LOOP : frontend " << _xx.name ;
  return CM_SUCCESS;
}

/*-- Frontend Loop -------------------------------------------------*/
INT frontend_loop() {
  int sleep_time(5);
  TLOG(TLVL_DEBUG+2) << " LOOP : frontend " << _xx.name << " sleep for extra " << sleep_time << " sec";
  ss_sleep(sleep_time);
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// at begin run want to clear all DTC counters
// has to be executed before the CFO
// call to 
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "BEGIN RUN : frontend " << _xx.name
                     << " _dtc_data[0].dtc_i =" << _dtc_data[0].dtc_i
                     << " _dtc_data[1].dtc_i =" << _dtc_data[1].dtc_i;
  
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "END RUN : frontend " << _xx.name ; 
  return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "PAUSE RUN : frontend " << _xx.name ;
  return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "RESUME RUN : frontend " << _xx.name ;
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
