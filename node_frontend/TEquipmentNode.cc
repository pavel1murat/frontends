//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"

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
  cm_get_experiment_database(&hDB, NULL);

  _odb_i                      = OdbInterface::Instance(hDB);
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

  TLOG(TLVL_DEBUG) << "active_run_conf:"  << active_run_conf 
                   << " public_subnet:"   << public_subnet
                   << " private subnet:"  << private_subnet 
                   << " _full_host_name:" << _full_host_name
                   << " _host_label:"     << _host_label
                   << std::endl
                   << "_monitorDtc:"      << _monitorDtc
                   << " _monitorArtdaq:"  << _monitorArtdaq;
  EqSetStatus("Started...", "white");
  fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  InitDtc();
  InitArtdaq();
  
  return TMFeOk();
}


//-----------------------------------------------------------------------------
// init DTC reaout for a given mode at begin run
// for now, assume that all DTCs are using the same ROC readout mode
// if this assumption will need to be dropped, will do that
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
      if (dtc_i) {
        dtc_i->fRocReadoutMode = roc_readout_mode;
        dtc_i->fEventMode      = event_mode;

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

  _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Dtc"         ,&_monitorDtc         );
  _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Artdaq"      ,&_monitorArtdaq      );
  _odb_i->GetInteger(_h_daq_host_conf,"Monitor/RocSPI"      ,&_monitorRocSPI      );
  _odb_i->GetInteger(_h_daq_host_conf,"Monitor/RocRegisters",&_monitorRocRegisters);
  
  TLOG(TLVL_DEBUG+1) << "_monitorDtc:" << _monitorDtc
                     << " _monitorRocSPI:" << _monitorRocSPI
                     << " _monitorRocRegisters:" << _monitorRocRegisters
                     << " _monitorArtdaq:" << _monitorArtdaq
                     << " TMFE::Instance()->fStateRunning:" << TMFE::Instance()->fStateRunning; 

  if (_monitorDtc) {
    ReadDtcMetrics();
  }

  if (_monitorArtdaq and TMFE::Instance()->fStateRunning) {
    ReadArtdaqMetrics();
  }
  
  char status[256];
  sprintf(status, "OK");
  EqSetStatus(status, "#00FF00");
}
