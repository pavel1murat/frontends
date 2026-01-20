//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <format>
#include "frontends/trk_cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

TEqTracker*  TEqTracker::fg_EqTracker;

//-----------------------------------------------------------------------------
TEqTracker::TEqTracker(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;
  fg_EqTracker            = this;
  _cmd_type               = kCmdUndefined;
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleInit(const std::vector<std::string>& args) {

  TLOG(TLVL_DEBUG) << "-- START";

  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  fEqConfBuffer = "SYSTEM";
//-----------------------------------------------------------------------------
// cache the ODB handle, as need to loop over the keys in InitArtdaq
//-----------------------------------------------------------------------------
  _odb_i                      = OdbInterface::Instance();
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  std::string private_subnet  = _odb_i->GetPrivateSubnet(_h_active_run_conf);
  std::string public_subnet   = _odb_i->GetPublicSubnet (_h_active_run_conf);
  std::string active_run_conf = _odb_i->GetRunConfigName(_h_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _host_label     = get_short_host_name(public_subnet.data());
  _full_host_name = get_full_host_name (private_subnet.data());

  _h_daq_host_conf = _odb_i->GetHostConfHandle(_host_label);

  _ss_ac_path      = "/Mu2e/ActiveRunConfiguration/Tracker";
  _h_tracker_conf  = _odb_i->GetHandle(0,_ss_ac_path);

  _ss_cmd_path     = "/Mu2e/Commands/Tracker";
//-----------------------------------------------------------------------------
// register hotlink
//-----------------------------------------------------------------------------
  HNDLE hdb        = _odb_i->GetDbHandle();
  HNDLE h_run      = _odb_i->GetHandle(0,_ss_cmd_path+"/Run");

  if (db_open_record(hdb, h_run,&_run,sizeof(_run), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    cm_msg(MERROR, __func__,"failed to open hotlink in ODB");
    TLOG(TLVL_ERROR) << "failed to open hotlink in ODB";
  }

  HNDLE h_test_run = _odb_i->GetHandle(0,"/Mu2e/Commands/Test/Run");

  if (db_open_record(hdb, h_test_run,&_run,sizeof(_run), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    cm_msg(MERROR, __func__,"failed to open hotlink for /Mu2e/Commands/Test/Run in ODB");
    TLOG(TLVL_ERROR) << "failed to open hotlink in ODB";
  }

  TLOG(TLVL_DEBUG) << "-- END";
  
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// init DTC reaout for a given mode at begin run
// for now, assume that all DTCs are using the same ROC readout mode
// if this assumption will need to be dropped, will do that
// may want to change the link mask at begin run w/o restarting the frontend
//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleBeginRun(int RunNumber)  {

  TLOG(TLVL_DEBUG) << "DONE";
  
  return TMFeOk();
};

//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}


//-----------------------------------------------------------------------------
// read DTC temperatures and voltages, artdaq metrics
// read ARTDAQ metrics only when running
//-----------------------------------------------------------------------------
void TEqTracker::HandlePeriodic() {

  TLOG(TLVL_DEBUG+1) << "--- START"; 
  TLOG(TLVL_DEBUG+1) << "--- END";

  EqSetStatus("OK","#00FF00");
}

//-----------------------------------------------------------------------------
// remember this is a static function...
// this is an early prototype of a command fanning-out logic
// for now, it is the responsibility of the command processor to set "/Finished"
// and , perhaps, "/ReturnCode"
// when the execution comes here, odb["/Mu2e/Commands/Tracker/Run"] is set to 1
//-----------------------------------------------------------------------------
void TEqTracker::ProcessCommand(int hDB, int hKey, void* Info) {

  OdbInterface* odb_i = OdbInterface::Instance();
  
  //   HNDLE h_trk_cmd = odb_i->GetTrackerCmdHandle();  // returns handle of '/Mu2e/Commands/Tracker'

  KEY key;

  db_get_key(hDB,hKey,&key);
  
  TLOG(TLVL_DEBUG) << std::format("--- START: hDB:{} hKey:{} k.name:{}",hDB,hKey,key.name); 
//-----------------------------------------------------------------------------
// get handle to the command sent to the tracker
//-----------------------------------------------------------------------------
  db_get_parent(hDB,hKey,&fg_EqTracker->_h_cmd);              // odb_i->GetHandle(hDB,hKey);  // returns handle 

  std::string logfile = odb_i->GetString(fg_EqTracker->_h_cmd,"logfile");

  if (odb_i->GetInteger(fg_EqTracker->_h_cmd,"Finished") == 0) {
    TLOG(TLVL_DEBUG) << "previous command not finished yet, return";
    fg_EqTracker->WriteOutput("-- ERROR: previous command not finished\n",logfile);
    return;
  }

  if (odb_i->GetInteger(fg_EqTracker->_h_cmd,"Run") == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    fg_EqTracker->WriteOutput("-- ERROR: self inflicted, return\n",logfile);
    return;
  }
//-----------------------------------------------------------------------------
// set tracker BUSY
//-----------------------------------------------------------------------------
  HNDLE h_trk_cfg  = odb_i->GetTrackerConfigHandle();
  odb_i->SetInteger(h_trk_cfg,"Status",1); // busy

  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  fg_EqTracker->_first_station   = first_station;
  fg_EqTracker->_last_station    = last_station ;

  std::string cmd                = odb_i->GetString(fg_EqTracker->_h_cmd,"Name");
  std::string cmd_parameter_path = odb_i->GetString(fg_EqTracker->_h_cmd,"ParameterPath");

  TLOG(TLVL_DEBUG) << std::format("cmd:{} cmd.parameter_path:{}",cmd,cmd_parameter_path);

  if      (cmd == "digi_rw"         ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "pulser_on"       ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "pulser_off"      ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "read"            ) fg_EqTracker->_cmd_type = kCmdDtc;  // "read" loads channel masks
  else if (cmd == "load_thresholds" ) fg_EqTracker->_cmd_type = kCmdDtc;
  else if (cmd == "print_status"    ) fg_EqTracker->_cmd_type = kCmdTracker;
  else if (cmd == "reset_output"    ) fg_EqTracker->_cmd_type = kCmdTracker;
  // ResetOutput(); // need to figure what to do
  else if (cmd == "reset_station_lv") fg_EqTracker->_cmd_type = kCmdRpi;
  else if (cmd == "test_command"    ) fg_EqTracker->_cmd_type = kCmdTracker;
  else {
    std::string msg = std::format("cmd:{} is not implemented yet",cmd);
    TLOG(TLVL_ERROR) << msg;
    cm_msg(MERROR, __func__,msg.data());
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
    WaitForCompletion(fg_EqTracker->_h_cmd);
  }

  int finished = odb_i->GetInteger(fg_EqTracker->_h_cmd,"Finished"  );
  int rc       = odb_i->GetInteger(fg_EqTracker->_h_cmd,"ReturnCode");
  
  TLOG(TLVL_DEBUG) << std::format("-- END finished:{} rc:{}",finished,rc);
  return;
}

//-----------------------------------------------------------------------------
int TEqTracker::WaitForCompletion(HNDLE h_Cmd) {
//-----------------------------------------------------------------------------
// wait for the command completion or timeout - this is a common part for all
// "per-DTC" commands
// for no-DTC commands it should be fast as all "ReturnCode" and "Finished"
// fields should check out during the first iteration
//-----------------------------------------------------------------------------
  int return_code(0);
  
  ss_sleep(100);
  // int finished = 0; // simulate .... 0;

  int wait_time(0);
  int n_not_finished = 100;
  
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
        
        int rc       = odb_i->GetInteger(h_dtc_cmd,"ReturnCode");
        int finished = odb_i->GetInteger(h_dtc_cmd,"Finished"  );
        if (finished == 1) {
                                        // assume rc can only be < 0
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
    odb_i->SetInteger(h_Cmd    ,"ReturnCode",-1);
    odb_i->SetInteger(h_trk_cfg,"Status"    ,-1);
  }
  else {
    odb_i->SetInteger(h_Cmd    ,"ReturnCode", return_code);
    odb_i->SetInteger(h_trk_cfg,"Status"    ,-return_code);
  }
//-----------------------------------------------------------------------------
// mark execution as completed
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_Cmd    ,"Finished",1);

  TLOG(TLVL_DEBUG) << std::format("-- END: n_not_finished:{} return_code:{}",n_not_finished,return_code);
  return n_not_finished;
}


//-----------------------------------------------------------------------------
// later, make sure that the tracekr inherits from TMu2eEqBase, not TMFEquipment
//-----------------------------------------------------------------------------
// make sure that a command can redirect its output
int TEqTracker::WriteOutput(const std::string& Output, const std::string& Logfile) {

  TLOG(TLVL_DEBUG) << std::format("-- START: Logfile:{}",Logfile); 

  std::vector<std::string> vs = splitString(Output,'\n');

  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  std::string fn = std::format("{}/{}",data_dir,Logfile);

  TLOG(TLVL_DEBUG) << std::format("using fn:{}",fn);
  
  std::ofstream output_file;
  output_file.open(fn.data(),std::ios::app);
  if (not output_file.is_open()) {
    TLOG(TLVL_ERROR) << std::format("failed to open log file:{} in ios::app mode",fn); 
  }
  else {
    int ns = vs.size();
    for (int i=ns-1; i>=0; i--) {
      output_file << vs[i] << std::endl;
      TLOG(TLVL_DEBUG+1) << vs[i];
    }
    output_file.close();
  }
  
  TLOG(TLVL_DEBUG) << "-- END"; 
  return 0;
}
