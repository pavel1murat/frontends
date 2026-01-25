//-----------------------------------------------------------------------------
// tracker configuration frontend
//-----------------------------------------------------------------------------
#ifndef __TEqTracker_hh__
#define __TEqTracker_hh__

#include "tmfe.h"
#include "midas.h"

#include "frontends/utils/OdbInterface.hh"
#include "frontends/utils/TMu2eEqBase.hh"

class TEqTracker : public TMu2eEqBase {
  enum {
    kCmdUndefined = 0,
    kCmdDtc       = 1,
    kCmdRpi       = 2,
    kCmdTracker   = 3,
  };
public:
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_tracker_conf;
  HNDLE                          _h_frontend_conf;
  std::string                    _full_host_name;  // on private network, for communicatioin
  std::string                    _host_label;      // on public network, for ODB
  OdbInterface*                  _odb_i;
  int                            _run;
  std::string                    _ss_ac_path;      // "/Mu2e/ActiveRunConfiguration/Tracker"
  std::string                    _ss_cmd_path;     // "/Mu2e/Commands/Tracker"
  int                            _first_station;
  int                            _last_station;
//-----------------------------------------------------------------------------
// commands
//-----------------------------------------------------------------------------
  HNDLE                          _h_cmd;
  int                            _cmd_type;        // command type: 1:DTC, 2:RPI, etc (see above)
  int                            _cmd_station;
  int                            _cmd_plane;
  int                            _cmd_panel;
//-----------------------------------------------------------------------------
// threads
//-----------------------------------------------------------------------------
  struct ThreadContext_t {
    int                             fPcieAddr;
    int                             fLink;
    int                             fRunning;      // status: 0=stopped 1=running
    int                             fStop;         // end marker
    int                             fCmd;          // command
    int                             fPrintLevel;

    ThreadContext_t() {}
    
    ThreadContext_t(int PcieAddr, int Link) {
      fPcieAddr = PcieAddr;
      fLink     = Link;
    }
  };

  static TEqTracker*                fg_EqTracker;
  ThreadContext_t                   fContext;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqTracker(const char* Name, const char* Title);

  virtual TMFeResult Init               () override;
//-----------------------------------------------------------------------------
// this is MIDAS callback
//-----------------------------------------------------------------------------
  static  void       ProcessCommand   (int hDB, int hKey, void* Info);
//-----------------------------------------------------------------------------
// fanout DTC command to all tracker DTCs and wait for completion
//-----------------------------------------------------------------------------
  static  int        ExecuteDtcCommand    (HNDLE hCmd);
  static  int        ExecuteRpiCommand    (HNDLE hCmd);
  static  int        ExecuteTrackerCommand(HNDLE hCmd);
  static  int        ExecuteTestCommand   (HNDLE hCmd);

  static  int        PulserOn         (const std::string& CmdParameterPath);
  static  int        PulserOff        (const std::string& CmdParameterPath);
  static  int        PanelPrintStatus (const std::string& CmdParameterPath);
  static  int        TestCommand      (int Station, int Plane, int Panel);
  static  TMFeResult ResetStationLV   (const std::string& CmdParameterPath);

  static  int        WaitForCompletion (HNDLE h_Cmd);

};
#endif
