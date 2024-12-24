///////////////////////////////////////////////////////////////////////////////
// DTC-ctl: DTC control frontend
// this is the only frontend which writes to the DTC registers
// -h host:port parameter points back to the MIDAS mserver
// the functionality is quite limited: 
// 1. configures the DTC (in a smart way, looking at the detector configuration) at startup
// 2. initializes the readout at begin run
// it doesn't need drivers....
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "trk_dtc_ctl_frontend"

#include <stdio.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
using namespace DTCLib;
using namespace trkdaq;

#include "dtc_frontend/trk_dtc_ctl_frontend.hh"
//-----------------------------------------------------------------------------
// Globals
// The frontend name (client name) as seen by other MIDAS clients, dei
//-----------------------------------------------------------------------------
const char* frontend_name;

namespace {
  class FeName {
  public:
    std::string name;

    FeName() { 
      name = get_short_host_name("local");
      name += "_trk_dtc_ctl";
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

  
  std::string     _log_fn ;
  std::ofstream*  _out;
  std::streambuf* _coutbuf;
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
    // char key[200];
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
std::string  get_logfile_name(std::string& output_dir) {

  char fn[200];
  sprintf(fn,"%s/logs/tfm_dtc_ctl/tfm_dtc_ctl.log");

  std::string s = fn;
  return s;
}
//-----------------------------------------------------------------------------
// Frontend Init
// use 'channel' as the DTC ID
//----------------------------------------------------------------------------- 
INT frontend_init() {
//-----------------------------------------------------------------------------
// get command line arguments - perhaps can use that one day
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "START";

  int         argc;
  char**      argv;
  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);
  _h_active_run_conf  = odb_i->GetActiveRunConfigHandle();
  _active_run_conf    = odb_i->GetRunConfigName(_h_active_run_conf);
//-----------------------------------------------------------------------------
// get logfile name
//-----------------------------------------------------------------------------
  std::string output_dir = odb_i->GetOutputDir(hDB);
  _log_fn = get_logfile_name(output_dir);

  _out = new  std::ofstream(_log_fn);
  
  _coutbuf  = std::cout.rdbuf();                  // save old buf
  std::cout.rdbuf(_out->rdbuf());                  // redirect std::cout to out.txt!
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _rpc_host  = get_short_host_name("local");

  TLOG(TLVL_DEBUG) << "config_name:" << _active_run_conf << " rpc_host:" << _rpc_host;

  _h_host_conf = odb_i->GetDaqHostHandle(hDB,_h_active_run_conf,_rpc_host);
//-----------------------------------------------------------------------------
// DTC is the equipment, two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
  HNDLE h_subkey;
  KEY   subkey;

  for (int i=0; db_enum_key(hDB,_h_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);

    TLOG(TLVL_DEBUG) << "subkey.name:" << subkey.name;

    if (strstr(subkey.name,"DTC") != subkey.name)           continue;

    int enabled        = odb_i->GetDtcEnabled    (hDB,h_subkey);
    int pcie_addr      = odb_i->GetPcieAddress   (hDB,h_subkey);
    
    memcpy(&equipment[_ndtcs],&eqq[pcie_addr],sizeof(EQUIPMENT));
    EQUIPMENT* eqp     = &equipment[_ndtcs];
    sprintf(eqp->name,"%s#DTC%i",_rpc_host.data(),pcie_addr);
    eqp->info.enabled  = (enabled == 1);

    TLOG(TLVL_DEBUG) << "DTC enabled:" << enabled << " pcie_addr:" << pcie_addr;
//-----------------------------------------------------------------------------
// the link mask will be defined by looping over the tracker panels
// ... later 
//-----------------------------------------------------------------------------
    int link_mask      = odb_i->GetDtcLinkMask   (hDB,h_subkey);
    TLOG(TLVL_DEBUG) << "link_mask:0x" << std::hex << link_mask << std::endl; 
//-----------------------------------------------------------------------------
// this is the only place where the DTC is fully initialized
// everywhere else should use skip_init = true
//-----------------------------------------------------------------------------
    bool skip_init          = false;
    DtcInterface* dtc_i     = DtcInterface::Instance(pcie_addr,link_mask,skip_init);
    _dtc_data[_ndtcs].dtc_i = dtc_i;
    _dtc_data[_ndtcs].h_dtc = h_subkey;
      
    dtc_i->fEnabled        = enabled;
// -----------------------------------------------------------------------------
// DTC readout mode: 0=ROC patterns , 1=DIGIs 2: new ROC patterns
// the readout mode is the same for all ROCs in the configuration
//-----------------------------------------------------------------------------
    dtc_i->fRocReadoutMode = odb_i->GetRocReadoutMode   (hDB,_h_active_run_conf);

    dtc_i->fEmulateCfo     = odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);
    dtc_i->fJAMode         = odb_i->GetDtcJAMode        (hDB,h_subkey);

    dtc_i->fEventMode      = odb_i->GetDtcEventMode     (hDB,h_subkey);
    dtc_i->fSampleEdgeMode = odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);

    dtc_i->fDtcID          = odb_i->GetDtcID            (hDB,h_subkey);
    dtc_i->fPartitionID    = odb_i->GetDtcPartitionID   (hDB,h_subkey);
    dtc_i->fOnSpill        = odb_i->GetDtcOnSpill       (hDB,h_subkey);
    dtc_i->fMacAddrByte    = odb_i->GetDtcMacAddrByte   (hDB,h_subkey);

    TLOG(TLVL_DEBUG) << "fRocReadoutMode:"  << dtc_i->fRocReadoutMode
                     << " fEmulateCfo:"     << dtc_i->fEmulateCfo
                     << " fJAMode:"         << dtc_i->fJAMode
                     << " fEventMode:"      << dtc_i->fEventMode
                     << " fSampleEdgeMode:" << dtc_i->fSampleEdgeMode;
    
    TLOG(TLVL_DEBUG) << " fDtcID:"          << dtc_i->fDtcID
                     << " fPartitionID:"    << dtc_i->fPartitionID
                     << " fOnSpill:"        << dtc_i->fOnSpill
                     << " fMacAddrByte:"    << dtc_i->fMacAddrByte;
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

//-----------------------------------------------------------------------------
// transitions
//-----------------------------------------------------------------------------
//  cm_register_transition(TR_START,tr_prestart,500);
  TLOG(TLVL_DEBUG) << "FINISHED, _ndtcs:" << _ndtcs;

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

  std::cout.rdbuf(_coutbuf);                //reset to standard output again
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
// at begin run, just initialize the readout assuming the DTCs have been configured 
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {
  int rc(0);
  TLOG(TLVL_DEBUG+2) << "BEGIN RUN : frontend " << _xx.name
                     << " _dtc_data[0].dtc_i =" << _dtc_data[0].dtc_i
                     << " _dtc_data[1].dtc_i =" << _dtc_data[1].dtc_i;

  for (int i=0; i<2; i++) {
    if (_dtc_data[0].dtc_i) {
      try         {
        _dtc_data[i].dtc_i->InitReadout();
        _dtc_data[i].dtc_i->PrintStatus();
        _dtc_data[i].dtc_i->PrintRocStatus();
        TLOG(TLVL_DEBUG) << "dtc:" << i << " supposedly initilized"
                         << " ROC readout mode:" <<  _dtc_data[i].dtc_i->RocReadoutMode();
      }
      catch (...) {
        TLOG(TLVL_ERROR) << "failed to initialize the DTC" << i << " on mu2edaq??";
        rc = -1;
      }
    }
  }
  
  if (rc == 0) return CM_SUCCESS;
  else         return CM_TRANSITION_CANCELED;
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
