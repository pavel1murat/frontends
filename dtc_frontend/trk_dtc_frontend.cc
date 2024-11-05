///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
// the DTC frontend is also responsible for properly initializing the DTCs on a given host
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "trk_dtc_flontend"

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
#include "dtc_frontend/emucfo_driver.hh"
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
  int           _ndtcs(0);                       // 0,1, or 2
  DtcInterface* _dtc_i[2] = {nullptr, nullptr};  //
}

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

  OdbInterface* odb_i         = OdbInterface::Instance(hDB);
  std::string active_run_conf = odb_i->GetActiveRunConfig(hDB);
  HNDLE h_active_run_conf     = odb_i->GetRunConfigHandle(hDB,active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
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

  int      cfo_pcie_addr(-1);
  uint64_t nevents      (0);
  int      ew_length    (0);
  uint64_t first_ew_tag (0);

  for (int i=0; db_enum_key(hDB,h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);

    TLOG(TLVL_DEBUG+2) << "subkey.name:" << subkey.name;

    if (strstr(subkey.name,"DTC") != subkey.name)           continue;

    int enabled          = odb_i->GetDtcEnabled    (hDB,h_subkey);
    int pcie_addr        = odb_i->GetPcieAddress   (hDB,h_subkey);
    
    EQUIPMENT* eqp = &equipment[pcie_addr];
    eqp->info.enabled  = (enabled == 1);

    TLOG(TLVL_DEBUG+2) << "DTC enabled today:" << enabled << " pcie_addr:" << pcie_addr;

    int link_mask     = odb_i->GetDtcLinkMask   (hDB,h_subkey);
    TLOG(TLVL_DEBUG+2) << "link_mask:0x" <<std::hex << link_mask << std::endl; 

    DtcInterface* dtc_i = DtcInterface::Instance(pcie_addr,link_mask);
    _dtc_i[pcie_addr]   = dtc_i;
      
    dtc_i->fEnabled        = enabled;
    dtc_i->fReadoutMode    = odb_i->GetDtcReadoutMode   (hDB,h_subkey);
    dtc_i->fSampleEdgeMode = odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
    dtc_i->fEmulateCfo     = odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);

    TLOG(TLVL_DEBUG+2) << "readout_mode:"      << dtc_i->fReadoutMode
                       << " sample_edge_mode:" << dtc_i->fSampleEdgeMode
                       << " emulate_cfo:"      << dtc_i->fEmulateCfo;

    if (enabled) {
      if (dtc_i->EmulateCfo()) {
        if (cfo_pcie_addr >= 0) {
          TLOG(TLVL_ERROR) << "redefinition of the emulated CFO" << std::endl;
        }
        else {
          cfo_pcie_addr = pcie_addr;
        }
      }
//-----------------------------------------------------------------------------
// there can be only one CFO, so don't worry about duplication
// enable emulated CFO, read parameters 
//-----------------------------------------------------------------------------
      nevents                  = odb_i->GetNEvents   (hDB,h_subkey);
      ew_length                = odb_i->GetEWLength  (hDB,h_subkey);
      first_ew_tag             = odb_i->GetFirstEWTag(hDB,h_subkey);
    }
//-----------------------------------------------------------------------------
// for each DTC, define a driver
// so far, output of all drivers goes into the same common "Input" array
// if a DTC runs in an emulated mode: define one more piece of equipment with a driver
//-----------------------------------------------------------------------------
    DEVICE_DRIVER* drv_list = new DEVICE_DRIVER[2];
    DEVICE_DRIVER* drv      = &drv_list[0];
    
    snprintf(drv->name,NAME_LENGTH,"dtc%i",i);
    drv->dd         = trk_dtc_driver;
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
  }

  if (cfo_pcie_addr >= 0) {
//-----------------------------------------------------------------------------
// after the DTCs are processed, add equipment called CFO with one driver
//-----------------------------------------------------------------------------
    EQUIPMENT* eqp = &equipment[2];
    
    eqp->info.enabled  = 1;
      
    DEVICE_DRIVER* drv_list  = new DEVICE_DRIVER[2];
    DEVICE_DRIVER* drv       = &drv_list[0];
    
    snprintf(drv->name,NAME_LENGTH,"emu_cfo");
    drv->dd         = emucfo_driver;
    drv->channels   = 1;                // fake, CFO driver is only needed to send event window markers
    drv->bd         = null;
    drv->flags      = DF_INPUT;
    drv->enabled    = true;

    DTC_DRIVER_INFO* ddi            = new DTC_DRIVER_INFO;
    ddi->driver_settings.pcieAddr   = cfo_pcie_addr;
    ddi->driver_settings.enabled    = 1;
    ddi->driver_settings.ewLength   = ew_length;
    ddi->driver_settings.nEvents    = nevents;
    ddi->driver_settings.firstEWTag = first_ew_tag;
    drv->dd_info                    = (void*) ddi;
    
    drv->mt_buffer  = nullptr;
    drv->pequipment = nullptr;

    drv_list[1]     = {"",};
    eqp->driver     = drv_list;
  }

//-----------------------------------------------------------------------------
// transitions
//-----------------------------------------------------------------------------
//  cm_register_transition(TR_START,tr_prestart,500);

  TLOG(TLVL_DEBUG+3) << "--- DONE";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// callback
// the DTC frontend is doing all configurations for the DTC-only mode, as well as
// when running in the emulated CFO mode
// in this mode th DTC fontend also sends the pulses in a loop function
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error)  {
  // code to perform actions prior to frontend starting 

  TLOG(TLVL_DEBUG+2) << "pre- BEGIN RUN : frontend " << _xx.name
                     << " _dtc_i[0]=" << _dtc_i[0] << " _dtc_i[1]=" << _dtc_i[1] ;
  
  for (int i=0; i<_ndtcs; i++) {
    trkdaq::DtcInterface* dtc_i = _dtc_i[i];
    if (dtc_i->Enabled()) {
      // initialize only enabled DTCs
      dtc_i->InitReadout(dtc_i->EmulateCfo(),dtc_i->fReadoutMode);
    }
  }
//-----------------------------------------------------------------------------
// at this point, the boardreaders are already running and waiting for events to show up
// if one of the DTCs is emulationa a CFO, enable calls to frontend_loop
//-----------------------------------------------------------------------------

  TLOG(TLVL_DEBUG+3) << "--- pre- BEGIN DONE";
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
