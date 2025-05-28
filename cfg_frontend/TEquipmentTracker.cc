//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include <format>
#include "frontends/cfg_frontend/TEquipmentTracker.hh"
#include "utils/utils.hh"
//#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentTracker"

//-----------------------------------------------------------------------------
TEquipmentTracker::TEquipmentTracker(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;
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

  _h_daq_host_conf = _odb_i->GetHostConfHandle(_h_active_run_conf,_host_label);

  _ac_path         = "/Mu2e/ActiveRunConfiguration";
  _ss_ac_path      = _ac_path+"/Tracker";

  _ss_cmd_path     = "/Mu2e/Commands/Tracker";

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
//-----------------------------------------------------------------------------
void TEquipmentTracker::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();

  std::string tracker_config_path = "/Mu2e/ActiveRunConfiguration/Tracker";
  midas::odb o_tracker_config(tracker_config_path);
  int nstations = o_tracker_config["NStations"];

  midas::odb o_tracker_cmd("/Mu2e/Commands/Tracker");
  std::string tracker_cmd        = o_tracker_cmd["Name"];
  std::string cmd_parameter_path = o_tracker_cmd["ParameterPath"];

  TLOG(TLVL_DEBUG) << "tracker_cmd:" << tracker_cmd
                   << " cmd_parameter_path:" << cmd_parameter_path
                   << " nstations:" << nstations << std::endl;

  if (tracker_cmd == "control_roc_pulser_on") {
//-----------------------------------------------------------------------------
// loop over all active ROCs and execute 'PULSER_ON'
// it woudl make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
    for (int is=0; is<nstations; ++is) {
      std::string station_path = std::format("{:s}/Station_{:02d}",tracker_config_path.data(),is);
      TLOG(TLVL_DEBUG) << " process station:" << is << " station_path:" << station_path;
      midas::odb o_station(station_path);
      if (o_station["Enabled"] == 0) continue;
      for (int pln=0; pln<2; ++pln) {
        std::string plane_path = std::format("{:s}/Plane_{:02d}",station_path.data(),pln);
        TLOG(TLVL_DEBUG) << "          process plane:" << pln << " plane_path:" << plane_path;
        midas::odb o_plane(plane_path);
        if (o_plane["Enabled"] == 0) continue;
        for (int pnl=0; pnl<6; ++pnl) {
          std::string panel_path = std::format("{:s}/Panel_{:02d}",plane_path.data(),pnl);
          TLOG(TLVL_DEBUG) << "          process panel:" << pnl << " panel_path:" << panel_path;
          midas::odb o_panel(panel_path);
          if (o_panel["Enabled"] == 0) continue;
//-----------------------------------------------------------------------------
// pulser_on for this panel: copy parameters to the DTC record
// need to figure the node name, DTC ID, and the link
//-----------------------------------------------------------------------------
          std::string name = o_panel["Name"];
          int mnid = atoi(name.substr(2).data());
          int sdir = (mnid/10)*10;
          std::string panel_map_path = std::format("/Mu2e/Subsystems/Tracker/PanelMap/{:03d}/{:s}",sdir,name.data());

          std::string dtc_path       = std::format("{:s}/DTC",panel_map_path.data());

          TLOG(TLVL_DEBUG) << " dtc_path:" << dtc_path;
          HNDLE h_dtc    = odb_i->GetHandle(0,dtc_path.data());
          HNDLE h_parent = odb_i->GetParent(h_dtc);
          
                                        // key.name is the frontend name ! 
          KEY node_fe;
          odb_i->GetKey(h_parent,&node_fe);
          // TLOG(TLVL_DEBUG) << " node_fe.name:" << (char*) node_fe.name;

          midas::odb o_dtc (panel_map_path+"/DTC");

          int pcie_addr = o_dtc  ["PcieAddress"];

          TLOG(TLVL_DEBUG) << "node_fe.name:" << (char*) node_fe.name
                           << " pcie_addr:" << pcie_addr;

          midas::odb o_trk_cmd("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on");

          std::string dtc_cmd_path = std::format("/Mu2e/Commands/Frontends/{:s}/DTC{:d}",
                                                 node_fe.name,pcie_addr);
          midas::odb o_dtc_cmd(dtc_cmd_path);
          
          int link         = o_trk_cmd["link"              ];
          int mask         = o_trk_cmd["first_channel_mask"];
          int duty_cycle   = o_trk_cmd["duty_cycle"        ];
          int pulser_delay = o_trk_cmd["pulser_delay"      ];
          int print_level  = o_trk_cmd["print_level"       ];

          TLOG(TLVL_DEBUG) << "dtc_cmd_path:" << dtc_cmd_path
                           << " link:" << link
                           << " mask:" << mask
                           << " duty_cycle:" << duty_cycle
                           << " pulser_delay:" << pulser_delay
                           << " print_level:" << print_level;

          o_dtc_cmd["Name"         ] = "control_roc_pulser_on";
          o_dtc_cmd["ParameterPath"] = cmd_parameter_path;
          
          // o_dtc_cmd["pulser_on"]["link"              ] = link;
          // o_dtc_cmd["pulser_on"]["first_channel_mask"] = mask;
          // o_dtc_cmd["pulser_on"]["duty_factor"       ] = duty_factor;
          // o_dtc_cmd["pulser_on"]["pulser_delay"      ] = pulser_delay;
          // o_dtc_cmd["pulser_on"]["print_level"       ] = print_level;
//-----------------------------------------------------------------------------
// finally, execute the command. THe loop is executed fast, so need to wait
// for the DTC's to report that they are finished
//-----------------------------------------------------------------------------
          o_dtc    ["Status"   ] = 1;
          o_dtc_cmd["Finished" ] = 0;
          o_dtc_cmd["Run"      ] = 1;
//-----------------------------------------------------------------------------
// wait till DTC comes back 
//-----------------------------------------------------------------------------
          ss_sleep(100);
          int finished = 0; // simulate .... 0;
          int wait_time(0);
          while ((finished == 0) and (wait_time < 10000)) {
            ss_sleep(100);
            finished = o_dtc_cmd["Finished"];
            wait_time += 100;
          }

          if (finished == 0) {
                                        // show DTC in ERROR
            o_dtc["Status"] = -1;
          }
          else {
            o_dtc["Status"] = 0;
          }
        }
      }
    }
    
  }
  else {
  }
                                        // just playing
  ss_sleep(100);
  o_tracker_cmd["Finished"] = 1;
  TLOG(TLVL_DEBUG) << "--- END"; 

}
