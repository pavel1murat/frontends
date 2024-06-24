///////////////////////////////////////////////////////////////////////////////
// a DTC frontend is always started on a node local to the DTC
// -h host:port parameter points back to the MIDAS mserver
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "cfo_em_frontend"

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
#include "cfo_frontend/cfo_em_frontend.hh"
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
BOOL        frontend_call_loop  =    FALSE; // frontend_loop is called periodically if this variable is TRUE
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
      name         += "cfo_em_frontend";
      frontend_name = name.data();  // frontend_name is a global variable
    }
  }; 
//-----------------------------------------------------------------------------
// need to figure how to get name, but that doesn't seem overwhelmingly difficult
//-----------------------------------------------------------------------------
  FeName         xx;
  // DEVICE_DRIVER  gen_driver[2];
  DEVICE_DRIVER  mon_driver[2];

  trkdaq::DtcInterface* _dtc_i          (nullptr);
  int                   _nEvents        (-1);
  int                   _eventWindowSize(-1);
}

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

  OdbInterface* odb_i = OdbInterface::Instance(hDB);
  odb_i->GetActiveRunConfig(hDB,active_run_conf);
  HNDLE h_active_run_conf = odb_i->GetRunConfigHandle(hDB,active_run_conf);

  std::string host        = get_full_host_name("local");
  HNDLE       h_cfo       = odb_i->GetCFOConfigHandle(hDB,h_active_run_conf);
  int         external    = odb_i->GetCFOExternal    (hDB,h_cfo);
  int         cfo_enabled = odb_i->GetCFOEnabled     (hDB,h_cfo);

  _nEvents                = odb_i->GetNEvents        (hDB,h_cfo);
  _eventWindowSize        = odb_i->GetEventWindowSize(hDB,h_cfo);
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
//-----------------------------------------------------------------------------
    int pcie_addr = odb_i->GetPcieAddress(hDB,h_cfo);
    _dtc_i        = DtcInterface::Instance(pcie_addr);
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

  return CM_SUCCESS;
}

/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  TLOG(TLVL_DEBUG+10) << "poll_event ENTERED";
  return 1;
}

//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
}

/*-- Frontend Exit -------------------------------------------------*/
INT frontend_exit() {
  TLOG(TLVL_DEBUG+10) << "called";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// Frontend Loop : for CFO, can't sleep here
//-----------------------------------------------------------------------------
INT frontend_loop() {
  TLOG(TLVL_DEBUG+10) << "frontend_loop ENTERED";
  return CM_SUCCESS;
}

//-----------------------------------------------------------------------------
// at begin run : send EWMs separatd by 
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+10) << "BEGIN RUN";

  _dtc_i->RocPatternConfig();
  _dtc_i->InitEmulatedCFOReadoutMode(_eventWindowSize,_nEvents+1,0);

  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/
INT end_of_run(INT run_number, char *error) {
  TLOG(TLVL_DEBUG+10) << "END RUN";
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
