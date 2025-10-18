//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEqCrvDtc.hh"
#include "node_frontend/TEqTrkDtc.hh"
#include "node_frontend/TEquipmentManager.hh"

#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"
#include "node_frontend/TEqArtdaq.hh"
#include "node_frontend/TEqDisk.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentManager"

TEquipmentManager* TEquipmentManager::gEqManager(nullptr);

//-----------------------------------------------------------------------------
TEquipmentManager::TEquipmentManager(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  TLOG(TLVL_DEBUG) << "-- START eqname:" << eqname << " eqfilename:" << eqfilename;

  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;

  gEqManager              = this;

  _eq_dtc[0]              = nullptr;
  _eq_dtc[1]              = nullptr;
  _eq_artdaq              = nullptr;
  _eq_disk                = nullptr;

  TLOG(TLVL_DEBUG) << " -- checkpoint 1";
  
  TLOG(TLVL_DEBUG) << "-- END";
}


//-----------------------------------------------------------------------------
// back to DTC: two are listed in the header, both should be listed in ODB
// can't do this in the constructor
//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::InitDtc() {

  TLOG(TLVL_DEBUG) << "--- START";

  _eq_dtc[0] = nullptr;
  _eq_dtc[1] = nullptr;
  
  HNDLE h_subkey;
  KEY   subkey;

  HNDLE hdb = _odb_i->GetDbHandle();
  for (int i=0; db_enum_key(hdb,_h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hdb,h_subkey,&subkey);
    
    TLOG(TLVL_DEBUG) << "subkey.name:" << subkey.name;
    
    if (strstr(subkey.name,"DTC") != subkey.name)           continue;
//-----------------------------------------------------------------------------
// capitalize the subsystem name
//-----------------------------------------------------------------------------
    std::string subsystem = _odb_i->GetString(h_subkey,"Subsystem");
    std::transform(subsystem.begin(),subsystem.end(),subsystem.begin(),::toupper);
    int dtc_enabled       = _odb_i->GetEnabled(h_subkey);
    int pcie_addr         = _odb_i->GetInteger(h_subkey,"PcieAddress");

    TLOG(TLVL_DEBUG) << "subsystem:" << subsystem << " enabled:" << dtc_enabled;
   
    if (dtc_enabled) {
      if      (subsystem == "CRV"    ) {
        _eq_dtc[pcie_addr] = new TEqCrvDtc(_h_active_run_conf,h_subkey);
      }
      else if (subsystem == "TRACKER") {
        _eq_dtc[pcie_addr]    = new TEqTrkDtc(_h_active_run_conf,h_subkey);
      }
    }
  }

  TLOG(TLVL_DEBUG) << "--- END: EQ_DTC0:" <<  _eq_dtc[0] << " EQ_DTC1:" <<  _eq_dtc[1];

  return TMFeOk();
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment
// a Mu2e DAQ node has:
// - up to 2 DTCs, depending on the subsystem, the DTCs could be running different firmwarw
// - artdaq processes running on it
// - standard node hardware (disk, memory etc)
//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleInit(const std::vector<std::string>& args) {

  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  fEqConfBuffer = "SYSTEM";

  _odb_i                      = OdbInterface::Instance();
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  std::string private_subnet  = _odb_i->GetPrivateSubnet(_h_active_run_conf);
  std::string public_subnet   = _odb_i->GetPublicSubnet (_h_active_run_conf);
  _full_host_name             = get_full_host_name (private_subnet.data());
  _host_label                 = get_short_host_name(public_subnet.data());
  _h_daq_host_conf            = _odb_i->GetHostConfHandle(_host_label);


  TLOG(TLVL_DEBUG) << "active_run_conf:"  << _odb_i->GetString(_h_active_run_conf,"Name")
                   << " public_subnet:"   << public_subnet
                   << " private subnet:"  << private_subnet 
                   << " _full_host_name:" << _full_host_name
                   << " _host_label:"     << _host_label;

  if (_h_daq_host_conf == 0) {
    // if we don't find the config, there is nothing we can do, abort
    char buf[256];
    sprintf(buf, "Host configuration DAQ/Nodes/%s not found", _host_label.c_str());
    fMfe->Msg(MERROR, "HandleInit", buf);
    return TMFeMidasError(buf,"TEquipmentManager::HandleInit(",DB_INVALID_NAME);
  }
  _h_frontend_conf = _odb_i->GetFrontendConfHandle(_h_active_run_conf,_host_label);
//-----------------------------------------------------------------------------
// in principle, CRV doesn't need to know about tracker-specific monitoring
// multiple options, but for now leave as is
//-----------------------------------------------------------------------------
  _diagLevel     = _odb_i->GetInteger(_h_frontend_conf,"DiagLevel");
  TLOG(TLVL_DEBUG) << "_diagLevel:"  << _diagLevel;

  EqSetStatus("Started...", "white");
  fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  InitDtc   ();
  InitArtdaq();
  InitDisk  ();
  
  TLOG(TLVL_DEBUG) << "-- END";
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// init DTC reaout for a given mode at begin run
// for now, assume that all DTCs are using the same ROC readout mode
// if this assumption will need to be dropped, will do that
// may want to change the link mask at begin run w/o restarting the frontend
//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleBeginRun(int RunNumber)  {

  int    handle_begin_run = _odb_i->GetInteger(_h_daq_host_conf,"Frontend/HandleBeginRun");

  TLOG(TLVL_DEBUG) << " handle_begin_run:" << handle_begin_run;
  int rc(0);
  
  if (handle_begin_run) {
    for (int i=0; i<2; i++) {
      if (_eq_dtc[i]) {
        // begin run returns either 0 (success) or a negative number
        rc -= _eq_dtc[i]->BeginRun(_h_active_run_conf);
      }
    }
  }
  
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;

  if (rc == 0) return TMFeOk();
  else {
    return TMFeResult(1,"failed to initialize DTC readout");
  }
};

//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// read DTC temperatures and voltages, artdaq metrics
// read ARTDAQ metrics only when running
//-----------------------------------------------------------------------------
void TEquipmentManager::HandlePeriodic() {

  TLOG(TLVL_DEBUG+1) << "-- START";

  //  double t  = TMFE::GetTime();
  // midas::odb::set_debug(true);

  // midas::odb o_runinfo("/Runinfo");
  // int running_state          = o_runinfo["State"];
  // int transition_in_progress = o_runinfo["Transition in progress"];
  //

  for (int pcie_addr=0; pcie_addr<2; pcie_addr++) {
    if (_eq_dtc[pcie_addr] and (_eq_dtc[pcie_addr]->MonitoringLevel() > 0)) {
      _eq_dtc[pcie_addr]->ReadMetrics();
    }
  }
//-----------------------------------------------------------------------------
// read metrics of artdaq processes running on this node
//-----------------------------------------------------------------------------
  if (_eq_artdaq and (_eq_artdaq->MonitoringLevel() > 0) and TMFE::Instance()->fStateRunning) {
    _eq_artdaq->ReadMetrics();
  }
//-----------------------------------------------------------------------------
// monitor node resources
//-----------------------------------------------------------------------------
  if (_eq_disk and (_eq_disk->MonitoringLevel() > 0)) {
    _eq_disk->ReadMetrics();
  }

  EqSetStatus(Form("OK"),"#00FF00");

  TLOG(TLVL_DEBUG+1) << "-- END";
}


//-----------------------------------------------------------------------------
// not sure if this function is needed any more
//-----------------------------------------------------------------------------
void TEquipmentManager::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();

  KEY k;
  odb_i->GetKey(hKey,&k);

  HNDLE h_dtc = odb_i->GetParent(hKey);
  KEY dtc;
  odb_i->GetKey(h_dtc,&dtc);

  int pcie_addr(0);
  if (dtc.name[3] == '1') pcie_addr = 1;

  HNDLE h_frontend = odb_i->GetParent(h_dtc);
  KEY frontend;
  odb_i->GetKey(h_frontend,&frontend);
  
  TLOG(TLVL_DEBUG) << "k.name:" << k.name
                   << " dtc.name:" << dtc.name
                   << " pcie_addr:" << pcie_addr
                   << " frontend.name:" << frontend.name;

  std::string dtc_cmd_buf_path = std::format("/Mu2e/Commands/Frontends/{}/{}",frontend.name,dtc.name);
  midas::odb o_dtc_cmd(dtc_cmd_buf_path);
                                        // should 0 or 1
  if (o_dtc_cmd["Run"] == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }

  std::string dtc_cmd        = o_dtc_cmd["Name"];
  std::string parameter_path = dtc_cmd_buf_path+std::format("/{:s}",dtc_cmd.data());
//-----------------------------------------------------------------------------
// 
// this is address of the parameter record
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "dtc_cmd:" << dtc_cmd;
  TLOG(TLVL_DEBUG) << "parameter_path:" << parameter_path;

  midas::odb o_par(parameter_path);
  TLOG(TLVL_DEBUG) << "-- parameters found";
//-----------------------------------------------------------------------------
// should be already defined at this point
//-----------------------------------------------------------------------------
  trkdaq::DtcInterface* dtc_i = trkdaq::DtcInterface::Instance(pcie_addr);

  if (dtc_cmd == "pulser_on") {
//-----------------------------------------------------------------------------
// execute pulser_on command , no printout
// so far assume link != -1, but we do want to use -1 (all)
//------------------------------------------------------------------------------
    int link               = o_par["link"              ];
    int first_channel_mask = o_par["first_channel_mask"];
    int duty_cycle         = o_par["duty_cycle"        ];
    int pulser_delay       = o_par["pulser_delay"      ];

    TLOG(TLVL_DEBUG) << "link:" << link
                     << " first_channel_mask:" << first_channel_mask
                     << " duty_cycle:" << duty_cycle
                     << " pulser_delay:" << pulser_delay;
    try {
      dtc_i->ControlRoc_PulserOn(link,first_channel_mask,duty_cycle,pulser_delay);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute ControlRoc_PulserON ... BAIL OUT";
    }
  }
  else if (dtc_cmd == "pulser_off") {
    int link               = o_par["link"       ];
    int print_level        = o_par["print_level"];

    TLOG(TLVL_DEBUG) << "link:" << link
                     << " print_level:" << print_level;
    try {
      dtc_i->ControlRoc_PulserOff(link,print_level);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute ControlRoc_PulserOFF ... BAIL OUT";
    }
  }
  
  o_dtc_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::InitArtdaq() {
  _eq_artdaq = new TEqArtdaq("artdaq");
  return TMFeOk();
}
//-----------------------------------------------------------------------------

TMFeResult TEquipmentManager::InitDisk() {
  _eq_disk = new TEqDisk("disk");
  return TMFeOk();
}
