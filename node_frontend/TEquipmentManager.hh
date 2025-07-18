//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TEquipmentManager_hh__
#define __TEquipmentManager_hh__

// #include "xmlrpc-c/config.h"  /* information about this build environment */
// #include <xmlrpc-c/base.h>
// #include <xmlrpc-c/client.h>
#include <ctime>
#include "tmfe.h"
#include "midas.h"

#include "utils/OdbInterface.hh"
#include "node_frontend/TMu2eEqBase.hh"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

class TEquipmentManager : public TMFeEquipment {
public:

  static TEquipmentManager*      gEqManager;

  TMu2eEqBase*                   _eq_dtc[2];   // if nullptr, then not defined
  TMu2eEqBase*                   _eq_artdaq;
  TMu2eEqBase*                   _eq_disk  ;

  std::vector<TMu2eEqBase*>      _eq_list  ;   // to iterate over all equipment items

  HNDLE                          hDB;                     // need to loop over ...
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_frontend_conf;
  //  HNDLE                          _h_dtc [2];
  std::string                    _full_host_name;  // on private network, for communicatioin
  std::string                    _host_label;      // on public network, for ODB
  //  xmlrpc_env                     _env;
  // int                            _monitorDtc;
  // int                            _monitorDisk;
  // int                            _monitorArtdaq;
  // int                            _monitorSPI;
  // int                            _monitorRocRegisters;
  // int                            _monitorRates;
  int                            _diagLevel;

  OdbInterface*                  _odb_i;
  int                            _dtc0_cmd_run; // RPC
  int                            _dtc1_cmd_run;
//-----------------------------------------------------------------------------
// threads
//-----------------------------------------------------------------------------
  struct ThreadContext_t {
    int                             fPcieAddr;
    int                             fLink;
    int                             fRunning;          // status: 0=stopped 1=running
    int                             fStop;             // end marker
    int                             fCmd;              // command
    int                             fPrintLevel;

    ThreadContext_t() {}
    
    ThreadContext_t(int PcieAddr, int Link, int PrintLevel = 0): 
      fPcieAddr (PcieAddr), fLink(Link), fPrintLevel(PrintLevel) {}
  };
  
  ThreadContext_t                   fSetThrContext;
  std::stringstream                 fSSthr;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEquipmentManager(const char* eqname, const char* eqfilename);

  TMFeResult         InitDtc            ();
  TMFeResult         InitArtdaq         ();
  TMFeResult         InitDisk           ();

  virtual TMFeResult HandleInit         (const std::vector<std::string>& args);
  virtual void       HandlePeriodic     ();
  virtual TMFeResult HandleRpc          (const char* cmd, const char* args, std::string& response);
  virtual TMFeResult HandleBinaryRpc    (const char* cmd, const char* args, std::vector<char>& response);
  virtual TMFeResult HandleBeginRun     (int RunNumber);
  virtual TMFeResult HandleEndRun       (int RunNumber);
  virtual TMFeResult HandlePauseRun     (int RunNumber);
  virtual TMFeResult HandleResumeRun    (int RunNumber);
  virtual TMFeResult HandleStartAbortRun(int RunNumber);

  static TEquipmentManager* Instance() { return gEqManager; }

  // ODB-based RPC command handler
  
  static void        ProcessCommand(int hDB, int hKey, void* Info);

};
#endif
