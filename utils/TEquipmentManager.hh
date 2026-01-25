//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TEquipmentManager_hh__
#define __TEquipmentManager_hh__

#include <ctime>
#include <sstream>
#include "tmfe.h"
#include "midas.h"

#include "utils/OdbInterface.hh"

// #include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

class TMu2eEqBase;

class TEquipmentManager : public TMFeEquipment {
public:

  static TEquipmentManager*      gEqManager;

                                               // includes only enabled equipment items - 
  std::vector<TMu2eEqBase*>      _eq_list  ;   // to iterate over them w/o additional checks

  HNDLE                          hDB;                     // need to loop over ...
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_frontend_conf;
  std::string                    _full_host_name;  // on private network, for communicatioin
  std::string                    _host_label;      // on public network, for ODB
  int                            _diagLevel;

  OdbInterface*                  _odb_i;
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

  TMu2eEqBase*       AddEquipmentItem(TMu2eEqBase* EqItem) {
    return _eq_list.emplace_back(EqItem);
  }

  TMu2eEqBase*       FindEquipmentItem(const std::string& Name);

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
