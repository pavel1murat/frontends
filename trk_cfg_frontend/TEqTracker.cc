//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <format>
#include <thread>

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

  fg_EqTracker            = this;
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
  _ss_ac_path      = "/Mu2e/ActiveRunConfiguration/Tracker";  // active conf path
  _handle          = _odb_i->GetHandle(0,_ss_ac_path);        // belongs to the base class

  _ss_cmd_path     = "/Mu2e/Commands/Tracker";                // command path
//-----------------------------------------------------------------------------
// register hotlink to /Mu2e/Commands/Tracker/Run
//-----------------------------------------------------------------------------
  HNDLE hdb        = _odb_i->GetDbHandle();
  HNDLE h_run      = _odb_i->GetHandle(0,_ss_cmd_path+"/Run");

  if (db_open_record(hdb, h_run,&_run,sizeof(_run), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
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
  TEqTracker* eq = TEqTracker::Instance();
  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_trk_cfg = odb_i->GetTrackerConfigHandle();  // returns handle of '/Mu2e/Commands/Tracker'

  int status = eq->GetStatus();
  if (status != 0) {
    TLOG(TLVL_WARNING) << std::format("status:{} 1=BUSY. BAIL OUT",status);
    return;
  }
//-----------------------------------------------------------------------------
// 1. set tracker status as BUSY, there is only one tracker
//-----------------------------------------------------------------------------
  eq->SetStatus(1);

  KEY key;
  db_get_key(hDB,hKey,&key);
  
  TLOG(TLVL_DEBUG) << std::format("hDB:{} hKey:{} k.name:{}",hDB,hKey,key.name); 
//-----------------------------------------------------------------------------
// get handle to the command sent to the tracker
//-----------------------------------------------------------------------------
  db_get_parent(hDB,hKey,&fg_EqTracker->_h_cmd);              // should return handle to /Mu2e/Commands/Tracker
  
  std::string cmd     = odb_i->GetString(fg_EqTracker->_h_cmd,"Name"   );
  std::string logfile = odb_i->GetString(fg_EqTracker->_h_cmd,"logfile");

  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  fg_EqTracker->_first_station   = first_station;
  fg_EqTracker->_last_station    = last_station ;

  std::string cmd_parameter_path = odb_i->GetString(fg_EqTracker->_h_cmd,"ParameterPath");

  TLOG(TLVL_DEBUG) << std::format("cmd:{} cmd.parameter_path:{}",cmd,cmd_parameter_path);

  if      (cmd == "digi_rw"         ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "find_alignment"  ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "init_readout"    ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "pulser_on"       ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "pulser_off"      ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "read"            ) fg_EqTracker->_cmd_type = kCmdDtc;  // "read" loads channel masks
  else if (cmd == "load_thresholds" ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "print_status"    ) fg_EqTracker->_cmd_type = kCmdTracker;
  else if (cmd == "reset_output"    ) {
//-----------------------------------------------------------------------------
// reset_output is a special command - it doesn't require looping over stations
// no need to wait for completion
//-----------------------------------------------------------------------------
    rc = fg_EqTracker->ResetOutput(logfile);
    odb_i->SetStatus(h_trk_cfg,rc);
    return;
  }
  else if (cmd == "reset_lv"        ) fg_EqTracker->_cmd_type = kCmdRpi;
  else if (cmd == "set_thresholds"  ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "test_command"    ) fg_EqTracker->_cmd_type = kCmdTracker;
  else {
    std::string msg = std::format("cmd:{} is not implemented yet",cmd);
    TLOG(TLVL_ERROR) << msg;
    cm_msg(MERROR, __func__,msg.data());
    cm_msg_flush_buffer();
  }
//-----------------------------------------------------------------------------
// first fan-out level: decide on the backend, the same backend may be
// responsible for processing several commands
// command execution can take some time, so need to be executed in a thread
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << std::format("cmd_type:{}",fg_EqTracker->_cmd_type);
  
  if (fg_EqTracker->_cmd_type != kCmdUndefined) {
    if      (fg_EqTracker->_cmd_type == kCmdDtc) {
      ExecuteDtcCommand(fg_EqTracker->_h_cmd);
    }
    else if (fg_EqTracker->_cmd_type == kCmdRpi) {
      ExecuteRpiCommand(fg_EqTracker->_h_cmd);
    }
    else if (fg_EqTracker->_cmd_type == kCmdTracker) {
                                        // no DTC, no RPI involved ?
      ExecuteTrackerCommand(fg_EqTracker->_h_cmd);
      // std::thread t(TEqTracker::ExecuteTrackerCommand,fg_EqTracker->_h_cmd);
      // t.detach();
    }
//-----------------------------------------------------------------------------
// make it a thread for wait not to interfere with the communication with MIDAS
//-----------------------------------------------------------------------------
    std::thread t(&TEqTracker::WaitForCompletion,fg_EqTracker->_h_cmd);
    t.detach();
  }

  // int finished = odb_i->GetInteger(fg_EqTracker->_h_cmd,"Finished"  );
  // int rc       = odb_i->GetInteger(fg_EqTracker->_h_cmd,"ReturnCode");
  
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return;
}

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
  
  ss_sleep(100);
  // int finished = 0; // simulate .... 0;

  int wait_time(0);
  int n_not_finished = 100;

  TEqTracker* eq = TEqTracker::Instance();
  
  OdbInterface* odb_i = OdbInterface::Instance();
  HNDLE h_trk_cfg   = odb_i->GetTrackerConfigHandle();
  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  HNDLE h_cmd_par   = odb_i->GetCmdParameterHandle(h_Cmd);
  std::string cmd   = odb_i->GetString (h_Cmd    ,"Name"      );
  int timeout_ms    = odb_i->GetInteger(h_cmd_par,"timeout_ms");

  TLOG(TLVL_DEBUG) << std::format("--- START: cmd:{} timeout_ms:{}",cmd,timeout_ms); 

  while ((n_not_finished > 0) and (wait_time < timeout_ms)) {
    ss_sleep(100);
    wait_time += 100;

    n_not_finished = 0;
    for (int is=first_station; is<last_station+1; ++is) {
      HNDLE h_station = odb_i->GetTrackerStationHandle(is);
      if (odb_i->GetEnabled(h_station) == 0) continue;
      for (int pln=0; pln<2; ++pln) {
        HNDLE h_plane = odb_i->GetTrackerPlaneHandle(is,pln);
        if (odb_i->GetEnabled(h_plane) == 0) continue;
        
        HNDLE       h_dtc     = odb_i->GetHandle        (h_plane,"DTC");
        int         pcie_addr = odb_i->GetDtcPcieAddress(h_dtc);
        std::string node      = odb_i->GetDtcHostLabel  (h_dtc);
        HNDLE       h_dtc_cmd = odb_i->GetDtcCmdHandle  (node,pcie_addr);
        
        int status = odb_i->GetInteger(h_dtc,"Status"  );
        int rc     = odb_i->GetInteger(h_dtc_cmd,"ReturnCode");
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
  if (n_not_finished != 0) {
    odb_i->SetInteger(h_Cmd,"ReturnCode",-1);
    eq->SetStatus(-1);
  }
  else {
    odb_i->SetInteger(h_Cmd,"ReturnCode", return_code);
    eq->SetStatus(-return_code);
  }
//-----------------------------------------------------------------------------
// mark execution as completed, don't touch 'Run' - no need
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_Cmd    ,"Finished",1);

  TLOG(TLVL_DEBUG) << std::format("-- END: n_not_finished:{} return_code:{}",n_not_finished,return_code);
  return n_not_finished;
}
