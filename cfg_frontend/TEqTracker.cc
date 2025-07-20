//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include <format>
#include "frontends/cfg_frontend/TEqTracker.hh"
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
  fg_EqTracker              = this;
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEqTracker::HandleInit(const std::vector<std::string>& args) {

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
    cm_msg(MERROR, __func__,"cannot open hotlink in ODB");
    //    return -1;
  }

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
  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE h_trk_cmd = odb_i->GetTrackerCommandHandle(); // returns handle of '/Mu2e/Commands/Tracker'
  if (odb_i->GetCommand_Run(h_trk_cmd) == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }

  HNDLE h_trk_cfg  = odb_i->GetTrackerConfigHandle();
  odb_i->SetInteger(h_trk_cfg,"Status",1); // busy

  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  fg_EqTracker->_first_station   = first_station;
  fg_EqTracker->_last_station    = last_station ;

  std::string tracker_cmd        = odb_i->GetString(h_trk_cmd,"Name");
  std::string cmd_parameter_path = odb_i->GetString(h_trk_cmd,"ParameterPath");

  cmd_parameter_path            += "/"+tracker_cmd;

  TLOG(TLVL_DEBUG) << "tracker_cmd:" << tracker_cmd
                   << " cmd_parameter_path:" << cmd_parameter_path;

  if      (tracker_cmd == "pulser_on"          ) {
    PulserOn (cmd_parameter_path);
    WaitForCompletion(h_trk_cmd,10000);
  }
  else if (tracker_cmd == "pulser_off"         ) PulserOff(cmd_parameter_path);
  else if (tracker_cmd == "panel_print_status" ) PanelPrintStatus(cmd_parameter_path);
  else if (tracker_cmd == "reset_output"       ) ResetOutput();
  else if (tracker_cmd == "reset_station_lv"   ) ResetStationLV(cmd_parameter_path);

  int finished = odb_i->GetInteger(h_trk_cmd,"Finished");
  
  TLOG(TLVL_DEBUG) << "--- END, o_tracker_cmd[\"Finished\"]:" << finished; 
}

//-----------------------------------------------------------------------------
int TEqTracker::WaitForCompletion(HNDLE h_Cmd, int TimeoutMs) {
//-----------------------------------------------------------------------------
// wait for the command completion or timeout - this is a common part for all
// "per-DTC" commands
// for no-DTC commands it should be fast as all "ReturnCode" and "Finished"
// fields should check out during the first iteration
//-----------------------------------------------------------------------------
  ss_sleep(100);
  // int finished = 0; // simulate .... 0;
  int wait_time(0);
  int n_not_finished = 100;
  
  OdbInterface* odb_i = OdbInterface::Instance();
  HNDLE h_trk_cfg   = odb_i->GetTrackerConfigHandle();
  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" ); 

  while ((n_not_finished > 0) and (wait_time < TimeoutMs)) {
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
        if ((rc != 0) or (finished == 0)) {
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
    odb_i->SetInteger(h_Cmd    ,"ReturnCode", 0);
    odb_i->SetInteger(h_trk_cfg,"Status"    ,-0);
  }
//-----------------------------------------------------------------------------
// mark execution as completed
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_trk_cfg,"Status"  ,0);
  odb_i->SetInteger(h_Cmd    ,"Finished",1);

  return n_not_finished;
}
