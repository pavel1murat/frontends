//////////////////////////////////////////////////////////////////////////////
// the tracker frontend fans out commands to other frontends
//////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <format>
#include <thread>
#include <chrono>

#include "frontends/utils/utils.hh"
#include "frontends/utils/TMu2eEqBase.hh"
#include "frontends/trk_cfg_frontend/TEqTracker.hh"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

TEqTracker*  TEqTracker::fg_EqTracker;
//-----------------------------------------------------------------------------
TEqTracker::TEqTracker(const char* Name, const char* Title): TMu2eEqBase(Name,Title,TMu2eEqBase::kTracker) {
  TLOG(TLVL_DEBUG) << "-- START";

  fg_EqTracker     = this;
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
  _handle          = _odb_i->GetTrackerConfigHandle();      // belongs to the base class
//-----------------------------------------------------------------------------
// register hotlink to /Mu2e/Commands/Tracker/Run
//-----------------------------------------------------------------------------
  HNDLE hdb        = _odb_i->GetDbHandle();
  HNDLE h_run      = _odb_i->GetHandle(0,"/Mu2e/Commands/Tracker/Run");

  if (db_open_record(hdb, h_run,&_cmd_run,sizeof(_cmd_run), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    cm_msg(MERROR, __func__,"failed to open hotlink in ODB");
    TLOG(TLVL_ERROR) << "failed to open hotlink in ODB";
  }

  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
TMFeResult TEqTracker::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// remember this is a static function...
// this is an early prototype of a command fanning-out logic
// for now, it is the responsibility of the command processor to set "/Finished"
// and , perhaps, "/ReturnCode"
// when the execution comes here, odb["/Mu2e/Commands/Tracker/Run"] is set to 1
//-----------------------------------------------------------------------------
void TEqTracker::ProcessCommand(int hDB, int hKey, void* Info) {
  int rc(0);
  TLOG(TLVL_DEBUG) << "-- START:";
//-----------------------------------------------------------------------------
// start from checking the tracker status
//-----------------------------------------------------------------------------
  TEqTracker*   eq    = TEqTracker::Instance();
  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_trk_cfg = eq->_handle; // odb_i->  // returns handle of '/Mu2e/ActiveRunConfiguration/Tracker'

  int status = eq->GetStatus();
  if (status != 0) {
    TLOG(TLVL_WARNING) << std::format("status:{} 1=BUSY. BAIL OUT",status);
    return;
  }
//-----------------------------------------------------------------------------
// 1. set tracker status as BUSY to block execution of the next commands until this
//    one is completed
//-----------------------------------------------------------------------------
  eq->SetStatus(1);

  KEY k_cmd;
  db_get_key(hDB,hKey,&k_cmd);

  HNDLE h_cmd = odb_i->GetParent(hKey);

  std::stringstream sstr;
  eq->StartMessage(h_cmd,sstr);
  
  
  TLOG(TLVL_DEBUG) << std::format("hDB:{} hKey:{} k_cmd.name:{} h_cmd:{}",hDB,hKey,k_cmd.name,h_cmd); 
//-----------------------------------------------------------------------------
// get handle to the command sent to the tracker
//-----------------------------------------------------------------------------
//   db_get_parent(hDB,hKey,&eq->_h_cmd);   // should return handle to odb_path=/Mu2e/Commands/Tracker
  
  std::string cmd     = odb_i->GetString(h_cmd,"Name"   );
  std::string logfile = odb_i->GetString(h_cmd,"logfile");

  eq->WriteOutput(sstr.str(),logfile,1);
  
  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  // do I really want to redefine this dynamically ? - potentially , a source of confusion
  eq->_first_station   = first_station;
  eq->_last_station    = last_station ;

  int cmd_type;
  
  if      (cmd == "digi_rw"         ) cmd_type = kCmdDtc;
  else if (cmd == "find_alignment"  ) cmd_type = kCmdDtc;
  else if (cmd == "init_readout"    ) cmd_type = kCmdDtc;
  else if (cmd == "load_thresholds" ) cmd_type = kCmdDtc;
  else if (cmd == "pulser_on"       ) cmd_type = kCmdDtc;
  else if (cmd == "pulser_off"      ) cmd_type = kCmdDtc;
  else if (cmd == "read"            ) cmd_type = kCmdDtc;  // "read" loads channel masks
  else if (cmd == "print_status"    ) cmd_type = kCmdTracker;
  else if (cmd == "reset_lv"        ) cmd_type = kCmdRpi;
  else if (cmd == "reset_output"    ) {
//-----------------------------------------------------------------------------
// execute right away
// reset_output is a special command - it doesn't require looping over stations
// no need to wait for completion
//-----------------------------------------------------------------------------
    rc = eq->ResetOutput(h_cmd);
    return;
  }
  else if (cmd == "set_thresholds"  ) cmd_type = kCmdDtc;
  else if (cmd == "test_command"    ) cmd_type = kCmdTracker;
  else                                cmd_type = kCmdUndefined;
//-----------------------------------------------------------------------------
// first fan-out level: decide on the backend, the same backend may be
// responsible for processing several commands
// command execution can take some time, so need to be executed in a thread
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << std::format("cmd_type:{}",cmd_type);
  
  if      (cmd_type == kCmdDtc) {
    eq->ExecuteDtcCommand(h_cmd);
  }
  else if (cmd_type == kCmdRpi) {
    eq->ExecuteRpiCommand(h_cmd);
  }
  else if (cmd_type == kCmdTracker) {
    // no DTC, no RPI involved - what it is?
    eq->ExecuteTrackerCommand(h_cmd);
    // std::thread t(TEqTracker::ExecuteTrackerCommand,h_cmd);
    // t.detach();
  }
  else {
    eq->UnknownCommand(h_cmd);
  }
//-----------------------------------------------------------------------------
// the tracker STATUS will be updated by WaitForCompletion
// make waiting a thread not to interfere with the communication with MIDAS
//-----------------------------------------------------------------------------
  std::thread t(&TEqTracker::WaitForCompletion,eq,h_cmd);
  t.detach();
  
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return;
}

//-----------------------------------------------------------------------------
// this is a static function - why? 
//-----------------------------------------------------------------------------
int TEqTracker::WaitForCompletion(HNDLE h_Cmd) {
//-----------------------------------------------------------------------------
// wait for the command completion or timeout - this is a common part for all
// "per-DTC" commands
// for no-DTC commands it should be fast as all "ReturnCode" and "Finished"
// fields should check out during the first iteration
// different tracker commands have different timeout values
//-----------------------------------------------------------------------------
  int return_code(0);
  
  //  ss_sleep(100);

  int wait_time(0), n_not_finished(100);

  HNDLE h_trk_cfg     = _odb_i->GetTrackerConfigHandle();
  int first_station   = _odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station    = _odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  HNDLE h_cmd_par   = _odb_i->GetCmdParameterHandle(h_Cmd);
  std::string cmd   = _odb_i->GetString (h_Cmd    ,"Name"      );
  int timeout_ms    = _odb_i->GetInteger(h_cmd_par,"timeout_ms");

  TLOG(TLVL_DEBUG) << std::format("--- START: cmd:{} timeout_ms:{}",cmd,timeout_ms); 

  while ((n_not_finished > 0) and (wait_time < timeout_ms)) {
    ss_sleep(100);
    wait_time += 100;

    n_not_finished = 0;
    for (int is=first_station; is<last_station+1; ++is) {
      HNDLE h_station = _odb_i->GetTrackerStationHandle(is);
      if (_odb_i->GetEnabled(h_station) == 0) continue;
      for (int pln=0; pln<2; ++pln) {
        HNDLE h_plane = _odb_i->GetTrackerPlaneHandle(is,pln);
        if (_odb_i->GetEnabled(h_plane) == 0) continue;
        
        HNDLE       h_dtc     = _odb_i->GetHandle        (h_plane,"DTC");
        int         pcie_addr = _odb_i->GetDtcPcieAddress(h_dtc);
        std::string node      = _odb_i->GetDtcHostLabel  (h_dtc);
        HNDLE       h_dtc_cmd = _odb_i->GetDtcCmdHandle  (node,pcie_addr);
        
        int status = _odb_i->GetInteger(h_dtc    ,"Status"    );
        int rc     = _odb_i->GetInteger(h_dtc_cmd,"ReturnCode");
        
        if (status == 0) {
                                        // completed
          if (rc != 0) return_code += rc;
        }
        else {
          n_not_finished += 1;
        }
      }
    }
    if (n_not_finished == 0) break;
  }
//-----------------------------------------------------------------------------
// either all finished, or timeout
//-----------------------------------------------------------------------------
  if (return_code == 0) {
    if (n_not_finished != 0) return_code = 1; // (BUSY) ... timeout
  }
  // do we really need the return code ? - perhaps Status would do ? 
  // odb_i->SetInteger (h_Cmd,"ReturnCode",return_code);

  SetCommandFinished(h_Cmd,return_code);

  TLOG(TLVL_DEBUG) << std::format("-- END: n_not_finished:{} return_code:{}",n_not_finished,return_code);
  return n_not_finished;
}

//-----------------------------------------------------------------------------
// for single-link commands aim for a 1 line output,
// tracker always has station, plane, panel
//-----------------------------------------------------------------------------
int TEqTracker::StartMessage(HNDLE H_Cmd, std::stringstream& Stream) {

  std::string cmd_name = _odb_i->GetString  (H_Cmd,"Name");
  int         station  = _odb_i->GetInteger(H_Cmd,"station");
  int         plane    = _odb_i->GetInteger(H_Cmd,"plane");
  int         mnid     = _odb_i->GetInteger(H_Cmd,"mnid");
  
  auto now = std::chrono::system_clock::now();
    
  // {:%Y-%m-%d %H:%M:%S} uses standard strftime-style flags
  std::string s_now = std::format("{:%Y-%m-%d %H:%M:%S}", now);

  Stream << std::format("{} -- TRACKER: cmd:{} station:{:02} plane:{:02} mnid:{:03d}\n", s_now,cmd_name,station,plane,mnid);
  return 0;
}

