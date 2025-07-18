//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include <format>
#include "frontends/cfg_frontend/TEquipmentTracker.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentTracker"

TEquipmentTracker*  TEquipmentTracker::fg_EqTracker;

//-----------------------------------------------------------------------------
TEquipmentTracker::TEquipmentTracker(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;
  fg_EqTracker              = this;
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEquipmentTracker::HandleInit(const std::vector<std::string>& args) {

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
TMFeResult TEquipmentTracker::HandleBeginRun(int RunNumber)  {

  TLOG(TLVL_DEBUG) << "DONE";
  
  return TMFeOk();
};

//-----------------------------------------------------------------------------
TMFeResult TEquipmentTracker::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentTracker::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentTracker::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentTracker::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}


//-----------------------------------------------------------------------------
// read DTC temperatures and voltages, artdaq metrics
// read ARTDAQ metrics only when running
//-----------------------------------------------------------------------------
void TEquipmentTracker::HandlePeriodic() {

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
void TEquipmentTracker::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE h_tracker_cmd = odb_i->GetCommandHandle("Tracker"); // returns handle of '/Mu2e/Commands/Tracker'
  if (odb_i->GetCommand_Run(h_tracker_cmd) == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }

  std::string tracker_config_path = "/Mu2e/ActiveRunConfiguration/Tracker";

  HNDLE h_tracker_config  = odb_i->GetHandle(0,tracker_config_path);
  odb_i->SetStatus(h_tracker_config,1);

  fg_EqTracker->_first_station   = odb_i->GetInteger(h_tracker_config,"FirstStation"); // o_tracker_config["FirstStation"];
  fg_EqTracker->_last_station    = odb_i->GetInteger(h_tracker_config,"LastStation" ); // o_tracker_config["LastStation" ];

  // std::string tracker_cmd        = o_tracker_cmd["Name"];
  // std::string cmd_parameter_path = o_tracker_cmd["ParameterPath"];

  std::string tracker_cmd        = odb_i->GetCommand_Name         (h_tracker_cmd);
  std::string cmd_parameter_path = odb_i->GetCommand_ParameterPath(h_tracker_cmd);

  cmd_parameter_path            += "/"+tracker_cmd;

  TLOG(TLVL_DEBUG) << "tracker_cmd:" << tracker_cmd
                   << " cmd_parameter_path:" << cmd_parameter_path;

  if      (tracker_cmd == "control_roc_pulser_on" ) ProcessCommand_PulserOn (cmd_parameter_path);
  else if (tracker_cmd == "control_roc_pulser_off") ProcessCommand_PulserOff(cmd_parameter_path);
  else if (tracker_cmd == "panel_print_status"    ) ProcessCommand_PanelPrintStatus(cmd_parameter_path);
  else if (tracker_cmd == "reset_output"          ) ProcessCommand_ResetOutput();
  else if (tracker_cmd == "reset_station_lv"      ) ProcessCommand_ResetStationLV(cmd_parameter_path);

//-----------------------------------------------------------------------------
// mark execution as completed
//-----------------------------------------------------------------------------
  odb_i->SetStatus(h_tracker_config,0);
  odb_i->SetCommand_Finished(h_tracker_cmd,1);
  int finished = odb_i->GetCommand_Finished(h_tracker_cmd);
  TLOG(TLVL_DEBUG) << "--- END, o_tracker_cmd[\"Finished\"]:" << finished; 
}
