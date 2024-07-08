///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
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
#include "cfo_frontend/cfo_mon_driver.hh"

using namespace DTCLib; 
using namespace CFOLib; 
using namespace std; 
using namespace trkdaq;
//-----------------------------------------------------------------------------
// Globals: used by the "base class" - $MYDASSYS/src/mfe.cxx
// The frontend name (client name) as seen by other MIDAS clients
//-----------------------------------------------------------------------------
const char* frontend_name;

const char* frontend_file_name  = __FILE__; // The frontend file name, don't change it
BOOL        frontend_call_loop  =   FALSE ; // frontend_loop is called periodically if this variable is TRUE
INT         display_period      =        0; // 1000; // if !=0, a frontend status page is displayed 
                                            //          with this frequency in ms
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
      name         += "cfo_emu_frontend";
      frontend_name = name.data();  // frontend_name is a global variable
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName         xx;
  // DEVICE_DRIVER  gen_driver[2];
  DEVICE_DRIVER  mon_driver[2];

  OdbInterface*         _odb_i          (nullptr);
  HNDLE                 _h_cfo ;

  trkdaq::DtcInterface* _dtc_i          (nullptr);
  long int              _nEvents        (-1);
  int                   _eventWindowSize(-1);
  long int              _firstEvent     ( 0);
  int                   _sleepTimeMs    (1000);
}

//-----------------------------------------------------------------------------
// callbacks
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error);
//-----------------------------------------------------------------------------
// CFO frontend init
//----------------------------------------------------------------------------- 
INT frontend_init() {
  int          argc;
  char**       argv;
//  int          sz;
  std::string  active_run_conf;
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

  active_run_conf         = _odb_i->GetActiveRunConfig(hDB);
  HNDLE h_active_run_conf = _odb_i->GetRunConfigHandle(hDB,active_run_conf);

  std::string host        = get_full_host_name("local");
  _h_cfo                  = _odb_i->GetCFOConfigHandle(hDB,h_active_run_conf);

  TLOG(TLVL_DEBUG+3) << "hDB : " << hDB << " _h_cfo: " << _h_cfo;

  int         cfo_enabled = _odb_i->GetCFOEnabled     (hDB,_h_cfo);
//-----------------------------------------------------------------------------
// now go to /Mu2e/DetectorConfigurations/$detector_conf/DAQ to get a list of
// nodes and DTC's to be monitored
// MIDAS 'host_name' could be 'local'..
// initialize the CFO
//-----------------------------------------------------------------------------
//  DEVICE_DRIVER*  drv;
  if (cfo_enabled == 1) {
//-----------------------------------------------------------------------------
// get the PCIE address and create the DTC interface
// skip initialization, assume the DTC to be initialized by the corresponding dtc_frontend
// skip_init=true skips initialization of the ROCs, so the initialization conflicts can only
// come via the initialization of the DMA engine
//-----------------------------------------------------------------------------
    int pcie_addr  = _odb_i->GetPcieAddress(hDB,_h_cfo);
    bool skip_init = true;
    _dtc_i         = DtcInterface::Instance(pcie_addr,0,skip_init);

    _nEvents                = _odb_i->GetNEvents        (hDB,_h_cfo);
    _eventWindowSize        = _odb_i->GetEventWindowSize(hDB,_h_cfo);
    _sleepTimeMs            = _odb_i->GetCFOSleepTime   (hDB,_h_cfo);

    TLOG(TLVL_DEBUG+2) << "_nEvents: " << _nEvents
                       << " _sleepTimeMs: " << _sleepTimeMs
                       << " _eventWindowSize: " << _eventWindowSize;
//-----------------------------------------------------------------------------
// emulated CFO - todo
//-----------------------------------------------------------------------------
    // drv->dd_info    = cdi;
    // drv->mt_buffer  = nullptr;
    // drv->pequipment = nullptr;
  }
//-----------------------------------------------------------------------------
// this is just to be able to use multidriver equipment type everywhere...
// could do a bit cleaner here with a single-driver equipment type
//-----------------------------------------------------------------------------
  mon_driver[0].name[0] = 0;
  equipment [0].driver  = mon_driver;
//-----------------------------------------------------------------------------
// transitions
//-----------------------------------------------------------------------------
  cm_register_transition(TR_START,tr_prestart,510);
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// callback (to be called AFTER teh corresponding DTC callback)
//-----------------------------------------------------------------------------
INT tr_prestart(INT run_number, char *error)  {
  // code to perform actions prior to frontend starting 

  TLOG(TLVL_DEBUG+2) << "pre-BEGIN RUN _dtc_i:" << _dtc_i;
  frontend_call_loop = TRUE ;
  TLOG(TLVL_DEBUG+3) << "--- pre-BEGIN DONE";
  return CM_SUCCESS;  
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
// Frontend Loop : for CFO, can't sleep here
//-----------------------------------------------------------------------------
INT frontend_loop() {

  TLOG(TLVL_DEBUG+2) << "--- ENTERED";
  if (_dtc_i) {
    TLOG(TLVL_DEBUG+2) << "_nEvents: " << _nEvents
                       << " _firstEvent: " << _firstEvent
                       << " _sleepTimeMs: " << _sleepTimeMs
                       << " _eventWindowSize: " << _eventWindowSize;

    _dtc_i->InitEmulatedCFOReadoutMode(_eventWindowSize,_nEvents+1,_firstEvent);
    _firstEvent += _nEvents;
  }

  ss_sleep(_sleepTimeMs);
  
  TLOG(TLVL_DEBUG+2) << "--- DONE after sleeping for " << _sleepTimeMs << " milliseconds";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// at begin run : send EWMs separatd by 
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  _firstEvent = 0;
  TLOG(TLVL_DEBUG+2) << "BEGIN RUN _firstEvent:" << _firstEvent;
  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "END RUN";
  frontend_call_loop = FALSE ;
  return CM_SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/
INT pause_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "PAUSE RUN";
  frontend_call_loop = FALSE ;
   return CM_SUCCESS;
}

/*-- Resume Run ----------------------------------------------------*/
INT resume_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+2) << "RESUME RUN";
  frontend_call_loop = TRUE ;
  return CM_SUCCESS;
}

/*------------------------------------------------------------------*/