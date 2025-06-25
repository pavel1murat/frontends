//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
TEquipmentNode::TEquipmentNode(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;

  fDtc_i[0]               = nullptr;
  fDtc_i[1]               = nullptr;
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleInit(const std::vector<std::string>& args) {

  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  fEqConfBuffer = "SYSTEM";

//-----------------------------------------------------------------------------
// cache the ODB handle, as need to loop over the keys in InitArtdaq
//-----------------------------------------------------------------------------
//  cm_get_experiment_database(&hDB, NULL);

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

  _h_daq_host_conf = _odb_i->GetHostConfHandle    (_h_active_run_conf,_host_label);
  if (_h_daq_host_conf == 0) {
    // if we don't find the config, there is nothing we can do, abort
    char buf[256];
    sprintf(buf, "Host configuration DAQ/Nodes/%s not found", _host_label.c_str());
    fMfe->Msg(MERROR, "HandleInit", buf);
    return TMFeMidasError(buf,"TEquipmentNode::HandleInit(",DB_INVALID_NAME);
  }
  _h_frontend_conf = _odb_i->GetFrontendConfHandle(_h_active_run_conf,_host_label);

  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Dtc"   ,&_monitorDtc   );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Artdaq",&_monitorArtdaq);
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/SPI"   ,&_monitorSPI);
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Rates" ,&_monitorRates );

  TLOG(TLVL_DEBUG) << "active_run_conf:"  << active_run_conf 
                   << " public_subnet:"   << public_subnet
                   << " private subnet:"  << private_subnet 
                   << " _full_host_name:" << _full_host_name
                   << " _host_label:"     << _host_label
                   << std::endl
                   << "_monitorDtc:"      << _monitorDtc
                   << "_monitorSPI:"      << _monitorSPI
                   << " _monitorArtdaq:"  << _monitorArtdaq
                   << " _monitorRates:"   << _monitorRates;
  EqSetStatus("Started...", "white");
  fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  InitDtc   ();
  InitArtdaq();
  InitDisk  ();
//-----------------------------------------------------------------------------
// hotlinks - start from one function handling both DTCs
//-----------------------------------------------------------------------------
  HNDLE hdb       = _odb_i->GetDbHandle();
  HNDLE h_dtc0    = _odb_i->GetHandle(0,std::format("/Mu2e/Commands/Frontends/{:s}/DTC0/Run",_host_label.data()));

  if (db_open_record(hdb,h_dtc0,&_run_dtc0_command,sizeof(int32_t), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    cm_msg(MERROR, __func__,"cannot open hotlink in ODB");
  }

  HNDLE h_dtc1   = _odb_i->GetHandle(0,std::format("/Mu2e/Commands/Frontends/{:s}/DTC1/Run",_host_label.data()));
  if (db_open_record(hdb,h_dtc1,&_run_dtc1_command,sizeof(int32_t), MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    cm_msg(MERROR, __func__,"cannot open hotlink in ODB");
  }
  
  return TMFeOk();
}


//-----------------------------------------------------------------------------
// init DTC reaout for a given mode at begin run
// for now, assume that all DTCs are using the same ROC readout mode
// if this assumption will need to be dropped, will do that
// may want to change the link mask at begin run w/o restarting the frontend
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleBeginRun(int RunNumber)  {

  //  HNDLE h_active_run_conf = _odb_i->GetActiveRunConfigHandle();
  int   event_mode        = _odb_i->GetEventMode     (_h_active_run_conf);
  int   roc_readout_mode  = _odb_i->GetRocReadoutMode(_h_active_run_conf);
  int    handle_begin_run(0);
  _odb_i->GetInteger(_h_daq_host_conf,"Frontend/HandleBeginRun",&handle_begin_run);

  TLOG(TLVL_DEBUG) << "event mode:"        << event_mode
                   << " roc_readout_mode:" << roc_readout_mode
                   << " handle_begin_run:" << handle_begin_run;

  if (handle_begin_run) {
    for (int i=0; i<2; i++) {
      mu2edaq::DtcInterface* dtc_i = fDtc_i[i];
      TLOG(TLVL_DEBUG) << "DTC" << i << ":" << dtc_i;
      if (dtc_i) {
        HNDLE h_dtc = _h_dtc[dtc_i->fPcieAddr];
        dtc_i->fEventMode      = event_mode;
        dtc_i->fRocReadoutMode = roc_readout_mode;
        dtc_i->fLinkMask       = _odb_i->GetLinkMask         (h_dtc);
        dtc_i->fJAMode         = _odb_i->GetJAMode           (h_dtc);
        dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(h_dtc);
//-----------------------------------------------------------------------------
// HardReset erases the DTC link mask, restore it
// also, release all buffers from the previous read - this is the initialization
//-----------------------------------------------------------------------------
        // 2025-01-19 PM dtc_i->Dtc()->HardReset();
        // 2025-01-19 PM dtc_i->ResetLinks(0,1);
                                        // InitReadout performs some soft resets, ok for now
        dtc_i->InitReadout();
      }
    }
  }
  TLOG(TLVL_DEBUG) << "DONE";
  
  return TMFeOk();
};

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}


//-----------------------------------------------------------------------------
// read DTC temperatures and voltages, artdaq metrics
// read ARTDAQ metrics only when running
//-----------------------------------------------------------------------------
void TEquipmentNode::HandlePeriodic() {

  TLOG(TLVL_DEBUG+1) << "-- START";

  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Dtc"         ,&_monitorDtc         );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Disk"        ,&_monitorDisk        );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Artdaq"      ,&_monitorArtdaq      );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/SPI"         ,&_monitorSPI         );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Rates"       ,&_monitorRates       );
  _odb_i->GetInteger(_h_frontend_conf,"Monitor/RocRegisters",&_monitorRocRegisters);
  
  TLOG(TLVL_DEBUG+1) << "_monitorDtc:"                      << _monitorDtc
                     << " _monitorSPI:"                     << _monitorSPI
                     << " _monitorRates:"                   << _monitorRates
                     << " _monitorRocRegisters:"            << _monitorRocRegisters
                     << " _monitorArtdaq:"                  << _monitorArtdaq
                     << " _monitorDisk:"                     << _monitorDisk
                     << " TMFE::Instance()->fStateRunning:" << TMFE::Instance()->fStateRunning; 

  if (_monitorDtc) ReadDtcMetrics();

  if (_monitorArtdaq and TMFE::Instance()->fStateRunning) {
    ReadArtdaqMetrics();
  }
  
  if (_monitorDisk) ReadDiskMetrics();

  TLOG(TLVL_DEBUG+1) << "-- END";

  EqSetStatus(Form("OK"),"#00FF00");
}

//-----------------------------------------------------------------------------
void TEquipmentNode::ProcessCommand(int hDB, int hKey, void* Info) {
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

  if (dtc_cmd == "control_roc_pulser_on") {
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
  else if (dtc_cmd == "control_roc_pulser_off") {
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

