//-----------------------------------------------------------------------------
// tracekr configuration frontend
//-----------------------------------------------------------------------------
#ifndef __TEquipmentTracker_hh__
#define __TEquipmentTracker_hh__

#include "tmfe.h"
#include "midas.h"

#include "frontends/utils/OdbInterface.hh"

class TEquipmentTracker : public TMFeEquipment {
public:
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_frontend_conf;
  std::string                    _full_host_name;  // on private network, for communicatioin
  std::string                    _host_label;      // on public network, for ODB
  OdbInterface*                  _odb_i;
  int                            _run;
  std::string                    _ac_path;         // "/Mu2e/ActiveRunConfiguration" (active config path)
  std::string                    _ss_ac_path;      // "/Mu2e/ActiveRunConfiguration/Tracker"
  std::string                    _ss_cmd_path;     // "/Mu2e/Commands/Tracker"
  int                            _first_station;
  int                            _last_station;
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
    
    ThreadContext_t(int PcieAddr, int Link) {
      fPcieAddr = PcieAddr;
      fLink     = Link;
    }
  };

  static TEquipmentTracker*         fg_EqTracker;
  ThreadContext_t                   fContext;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEquipmentTracker(const char* eqname, const char* eqfilename);

  virtual TMFeResult HandleInit         (const std::vector<std::string>& args);
  virtual void       HandlePeriodic     ();
  // virtual TMFeResult HandleRpc          (const char* cmd, const char* args, std::string& response);
  // virtual TMFeResult HandleBinaryRpc    (const char* cmd, const char* args, std::vector<char>& response);
  virtual TMFeResult HandleBeginRun     (int RunNumber);
  virtual TMFeResult HandleEndRun       (int RunNumber);
  virtual TMFeResult HandlePauseRun     (int RunNumber);
  virtual TMFeResult HandleResumeRun    (int RunNumber);
  virtual TMFeResult HandleStartAbortRun(int RunNumber);
  
  static  void       ProcessCommand(int hDB, int hKey, void* Info);

  static  void       ProcessCommand_PulserOn        (const std::string& CmdParameterPath);
  static  void       ProcessCommand_PulserOff       (const std::string& CmdParameterPath);
  static  void       ProcessCommand_PanelPrintStatus(const std::string& CmdParameterPath);
  static  void       ProcessCommand_ResetOutput();
  static  TMFeResult ProcessCommand_ResetStationLV  (const std::string& CmdParameterPath);


};
#endif
