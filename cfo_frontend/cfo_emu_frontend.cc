///////////////////////////////////////////////////////////////////////////////
// emulated CFO frontend just defines its driver
// as the equipment is defined as SLOW_CONTROL equipment, the driver is called
// periodically, bypassing the user part of the frontend
// at run-time, the driver generates trains of pulses
// the frontend is not doing anything except defining the run-time
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "cfo_emu_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "cfoInterfaceLib/CFO.h"
#include "dtcInterfaceLib/DTC.h"

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"

#include "cfo_frontend/cfo_interface.hh"
#include "cfo_frontend/cfo_emu_frontend.hh"
// #include "cfo_frontend/cfo_emu_driver.hh"

using namespace DTCLib; 
using namespace CFOLib; 
using namespace std; 
using namespace trkdaq;
//-----------------------------------------------------------------------------
// Globals: used by the "base class" - $MYDASSYS/src/mfe.cxx
// The frontend name (client name) as seen by other MIDAS clients
//-----------------------------------------------------------------------------

int cfo_emu_frontend::running = 0;

const char* frontend_name;

const char* frontend_file_name  = __FILE__; // The frontend file name, don't change it
BOOL        frontend_call_loop  =    TRUE ; // frontend_loop is called periodically if this variable is TRUE
INT         display_period      =        0; // 1000; // if !=0, a frontend status page is displayed 
                                            //          with this frequency in ms
INT         max_event_size      =    10000; // maximum event size produced by this frontend
INT         max_event_size_frag =  5*10000; // maximum event size for fragmented events (EQ_FRAGMENTED)
INT         event_buffer_size   = 10*10000; // buffer size to hold events */

int        _cfo_running;
//-----------------------------------------------------------------------------
// local scope variables
//-----------------------------------------------------------------------------
namespace {
  class FeName {
    std::string name;
  public:
    FeName() { 
      name         += "cfo_emu_fe";
      frontend_name = name.data();  // frontend_name is a global variable
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName         xx;
  //  DEVICE_DRIVER  mon_driver[2];

  OdbInterface*         _odb_i          (nullptr);
  HNDLE                 _h_cfo ;
  int                   _cfo_enabled;
  int                   _n_ewm_train;         // N(EWM's per emulated CFO pulse train)
  int                   _ew_length;           // EW length in 25ns ticks
  ulong                 _first_ts;
  int                   _pcie_addr;
  int                   _sleep_time_ms;
  trkdaq::DtcInterface* _dtc_i(nullptr);
}

//-----------------------------------------------------------------------------
// CFO frontend init
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int          argc;
  char**       argv;
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB 0 
// hDB is a global defined in $MIDASSYS/src/mfe.cxx
// h_active_run_conf is the key corresponding the the active run configuration
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  _odb_i = OdbInterface::Instance(hDB);
//-----------------------------------------------------------------------------
// the CFO frontend starts after the DTC frontends (500) and stops before them
//-----------------------------------------------------------------------------
  cm_set_transition_sequence(TR_START,520);
  cm_set_transition_sequence(TR_STOP ,480);
  
  TLOG(TLVL_DEBUG) << "END";
  return CM_SUCCESS;
}


//-----------------------------------------------------------------------------
int cfo_emu_launch_run_plan(char *pevent, int) {
  TLOG(TLVL_DEBUG+1) << "START" ;
  //  _dtc_i->LaunchRunPlanEmulatedCfo(_ew_length,_n_ewm_train+1,_first_ts);

  TLOG(TLVL_DEBUG) << " _ew_length:"     << _ew_length
                   << "_n_ewm_train:"    << _n_ewm_train
                   << " _first_ts:"      << _first_ts;
  
  _dtc_i->LaunchRunPlanEmulatedCfo(_ew_length,_n_ewm_train+1,_first_ts);
  _first_ts += _n_ewm_train;
  
  TLOG(TLVL_DEBUG+1) << "END" ;
  return 0;
}

/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  TLOG(TLVL_DEBUG+2) << "poll_event ENTERED";
  return 1;
}

//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit() {
  TLOG(TLVL_DEBUG+2) << "called";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// sleep for a while , this frontend doesn't have much to do
//-----------------------------------------------------------------------------
INT frontend_loop() {
  TLOG(TLVL_DEBUG+2) << "frontend_loop ENTERED";
  if (_sleep_time_ms > 1) {
    ss_sleep(_sleep_time_ms-1);
  }
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// can afford to re-initialize the run plan at each begin run
// active run configuration can change from one run to another
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {

  TLOG(TLVL_DEBUG) << "BEGIN RUN " << run_number;

  std::string  active_run_conf = _odb_i->GetActiveRunConfig(hDB);

  HNDLE h_active_run_conf = _odb_i->GetRunConfigHandle(hDB,active_run_conf);

  std::string host        = get_full_host_name("local");
  _h_cfo                  = _odb_i->GetCFOConfigHandle(hDB,h_active_run_conf);
  _cfo_enabled            = _odb_i->GetCFOEnabled     (hDB,_h_cfo);

  _n_ewm_train   = _odb_i->GetCFONEventsPerTrain(hDB,_h_cfo);
  _ew_length     = _odb_i->GetEWLength    (hDB,_h_cfo);
  _first_ts      = _odb_i->GetFirstEWTag  (hDB,_h_cfo);            // normally, start from zero
  _sleep_time_ms = _odb_i->GetCFOSleepTime(hDB,_h_cfo);
//----------------------------------------------------------------------------- 
// we know that this is an emulated CFO - get pointer to the corresponding DTC
// an emulated CFO configuration includs a link to the DTC
//-----------------------------------------------------------------------------
  HNDLE h_dtc    = _odb_i->GetHandle     (hDB,_h_cfo,"DTC");
  _pcie_addr     = _odb_i->GetPcieAddress(hDB, h_dtc);
//-----------------------------------------------------------------------------
// don't initialize the DTC, just get a pointer to
//-----------------------------------------------------------------------------
  _dtc_i         = trkdaq::DtcInterface::Instance(_pcie_addr,0,true);
  int event_mode = _odb_i->GetCFOEventMode(hDB,_h_cfo);
  _dtc_i->SetEventMode(event_mode);

  TLOG(TLVL_DEBUG) << "active_run_conf:" << active_run_conf
                   << " hDB : " << hDB   << " _h_cfo: " << _h_cfo
                   << " cfo_enabled: "   << _cfo_enabled
                   << "h_dtc:"           << h_dtc
                   << "_pcie_addr: "     << _pcie_addr;

  TLOG(TLVL_DEBUG) << "_n_ewm_train:"    << _n_ewm_train
                   << " _ew_length:"     << _ew_length
                   << " _first_ts:"      << _first_ts
                   << " _sleep_time_ms:" << _sleep_time_ms;
  cfo_emu_frontend::running = 1;

  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "END RUN " << run_number;
  cfo_emu_frontend::running = 0;
  return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "PAUSE RUN";
  cfo_emu_frontend::running = 0;
   return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "RESUME RUN";
  cfo_emu_frontend::running = 0;
   return CM_SUCCESS;
}

/*------------------------------------------------------------------*/
