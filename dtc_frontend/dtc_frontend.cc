///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
// the DTC frontend is also responsible for properly initializing the DTCs on a given host
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
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "dtc_frontend/dtc_frontend.hh"

using namespace trkdaq;
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
  int           _ndtcs(0);                       // 0,1, or 2
  DtcInterface* _dtc_i[2] = {nullptr, nullptr};  //
}

static DEVICE_DRIVER* _driver_list (nullptr);


const char *frontend_file_name = __FILE__; // The frontend file name, don't change it

BOOL frontend_call_loop         = FALSE; // TRUE;     // frontend_loop is called periodically if this variable is TRUE
INT  display_period             = 1000;     // a frontend status page is displayed with this frequency in ms
INT  max_event_size             = 10000;    // maximum event size produced by this frontend
INT  max_event_size_frag        = 5*10000;  // maximum event size for fragmented events (EQ_FRAGMENTED)
INT  event_buffer_size          = 10*10000; // buffer size to hold events */

//-----------------------------------------------------------------------------
// callbacks
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error);
//-----------------------------------------------------------------------------
// Frontend Init
// use 'channel' as the DTC ID
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int         argc;
  char**      argv;
  std::string active_run_conf;
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);

  active_run_conf         = odb_i->GetActiveRunConfig(hDB);
  HNDLE h_active_run_conf = odb_i->GetRunConfigHandle(hDB,active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/DetectorConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  std::string rpc_host  = get_short_host_name("local");

  TLOG(TLVL_DEBUG+2) << "rpc_host:" << rpc_host;

  HNDLE h_daq_host_conf = odb_i->GetDaqHostHandle(hDB,h_active_run_conf,rpc_host);
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

    TLOG(TLVL_DEBUG+2) << "subkey.name:" << subkey.name;

    if (strstr(subkey.name,"DTC") != subkey.name)           continue;

    int enabled          = odb_i->GetDtcEnabled    (hDB,h_subkey);

    TLOG(TLVL_DEBUG+2) << "DTC enabled:" << enabled;

    int pcie_addr        = odb_i->GetPcieAddress   (hDB,h_subkey);

    equipment[pcie_addr].info.enabled  = (enabled == 1);
    
    if (enabled) {
      int link_mask     = odb_i->GetDtcLinkMask   (hDB,h_subkey);
      _dtc_i[pcie_addr] = DtcInterface::Instance(pcie_addr,link_mask);
      
      _dtc_i[pcie_addr]->fReadoutMode    = odb_i->GetDtcReadoutMode   (hDB,h_subkey);
      _dtc_i[pcie_addr]->fSampleEdgeMode = odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
      _dtc_i[pcie_addr]->fEmulatesCfo    = odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);
    }

    TLOG(TLVL_DEBUG+2) << "readout_mode:"      << _dtc_i[pcie_addr]->fReadoutMode
                       << " sample_edge_mode:" << _dtc_i[pcie_addr]->fSampleEdgeMode
                       << " emulates_cfo:"     << _dtc_i[pcie_addr]->fEmulatesCfo;
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

    equipment[pcie_addr].driver = _driver_list;
  }
//-----------------------------------------------------------------------------
// transitions
//-----------------------------------------------------------------------------
  cm_register_transition(TR_START,tr_prestart,500);

  TLOG(TLVL_DEBUG+3) << "--- DONE";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// callback
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error)  {
  // code to perform actions prior to frontend starting 

  TLOG(TLVL_DEBUG+2) << "pre-BEGIN RUN : frontend " << _xx.name
                     << " _dtc_i[0]=" << _dtc_i[0] << " _dtc_i[1]=" << _dtc_i[1] ;
  
  for (int i=0; i<_ndtcs; i++) {
    trkdaq::DtcInterface* dtc_i = _dtc_i[i];
    dtc_i->Dtc()->SoftReset();
    if (dtc_i->RocReadoutMode() == 0) {
//-----------------------------------------------------------------------------
// ROC-generated patterns
//-----------------------------------------------------------------------------
      dtc_i->RocConfigurePatternMode(dtc_i->fLinkMask);  // per run, need just a piece...
    }
    else if (dtc_i->RocReadoutMode() == 1) {
//-----------------------------------------------------------------------------
// need to configure the ROCs to do the right thing
//-----------------------------------------------------------------------------
    }

//-----------------------------------------------------------------------------
// in emulated mode, the DTC configuration is done by the cfo_emu_frontend
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG+2) << "emulatesCfo: " << dtc_i->EmulatesCfo();

    if (dtc_i->EmulatesCfo() == 0) {
      dtc_i->InitExternalCFOReadoutMode(dtc_i->fSampleEdgeMode);
    }
  }

  TLOG(TLVL_DEBUG+3) << "--- pre-BEGIN DONE";
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
                     << " _dtc_i[0]=" << _dtc_i[0] << " _dtc_i[1]=" << _dtc_i[1] ;
  
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
