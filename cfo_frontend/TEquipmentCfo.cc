//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "cfo_frontend/TEquipmentCfo.hh"
#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentCfo"

//-----------------------------------------------------------------------------
TEquipmentCfo::TEquipmentCfo(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;

  _cfo_i                  = nullptr;
}

//-----------------------------------------------------------------------------
int TEquipmentCfo::InitEmulatedCfo() {
  int rc(0);
//----------------------------------------------------------------------------- 
// we know that this is an emulated CFO - get pointer to the corresponding DTC
// an emulated CFO configuration includs a link to the DTC
//-----------------------------------------------------------------------------
  HNDLE h_dtc    = _odb_i->GetHandle     (_h_cfo,"DTC");
  _pcie_addr     = _odb_i->GetDtcPcieAddress(h_dtc);
//-----------------------------------------------------------------------------
// get a pointer to the underlying interface to DTC and initialize its parameters
// emulated CFO interface doesn't re-initialize the DTC
//-----------------------------------------------------------------------------
  bool skip_init = true;
  _dtc_i         = trkdaq::DtcInterface::Instance(_pcie_addr,0,skip_init);

  _dtc_i->SetEventMode(_event_mode);

  _dtc_i->fPcieAddr       = _pcie_addr;
  _dtc_i->fDtcID          = _odb_i->GetDtcID            (h_dtc);
  _dtc_i->fLinkMask       = _odb_i->GetLinkMask         (h_dtc);
//-----------------------------------------------------------------------------
// for the emulated CFO, the underlying DTC may, in principle, be enabled,
// while the CFO itself is disabled
// not sure what running mode that would represent though
//-----------------------------------------------------------------------------
  _dtc_i->fEnabled        = _odb_i->GetEnabled          (h_dtc);
  _dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(h_dtc);
  _dtc_i->fRocReadoutMode = _odb_i->GetRocReadoutMode   (_h_active_run_conf);
  _dtc_i->fJAMode         = _odb_i->GetJAMode           (h_dtc);
  _dtc_i->fMacAddrByte    = _odb_i->GetDtcMacAddrByte   (h_dtc);
  
  TLOG(TLVL_DEBUG) << " _h_cfo: "        << _h_cfo
                   << " h_dtc:"          << h_dtc
                   << "_pcie_addr: "     << _pcie_addr ;
  TLOG(TLVL_DEBUG) << "_n_ewm_train:"    << _n_ewm_train
                   << " _ew_length:"     << _ew_length
                   << " _first_ts:"      << _first_ts
                   << " _sleep_time_ms:" << _sleep_time_ms;
  return rc;
}


//-----------------------------------------------------------------------------
int TEquipmentCfo::InitExternalCfo() {
  int rc(0);
//---------------------------------------------------------------------------
// in principle, can compile the run plan on the fly, make it a next step
// CfoGetRunPlan returns a string - filename of the compiled run plan file 
//---------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "--- START";
//-----------------------------------------------------------------------------
// 'external' CFO is a separate hardware unit, and that can't be enabled if
// the CFO is disabled in the run configuration
//-----------------------------------------------------------------------------
  std::string run_plan_dir = _odb_i->GetString(0,"/Mu2e/CFO_RUN_PLAN_DIR");
  if (run_plan_dir[0] == '$') run_plan_dir = getenv(run_plan_dir.substr(1).data());

  _run_plan_fn = run_plan_dir+'/'+_odb_i->GetCfoRunPlan(_h_cfo);
//----------------------------------------------------------------------------- 
// we know that this is an emulated CFO - get pointer to the corresponding DTC
// an emulated CFO configuration includs a link to the DTC
// "/DAQ/CFO/CFO" is a link to the hardware CFO
//-----------------------------------------------------------------------------
  HNDLE h_cfo_cfo  = _odb_i->GetHandle        (_h_cfo,"CFO");
  _pcie_addr       = _odb_i->GetDtcPcieAddress(h_cfo_cfo);
  _link_mask       = _odb_i->GetLinkMask      (_h_cfo);
  
  _cfo_i           = trkdaq::CfoInterface::Instance(_pcie_addr,_link_mask);
  _cfo_i->fJAMode  = _odb_i->GetJAMode        (_h_cfo);
  _cfo_i->fEnabled = _enabled;
  _cfo_i->SetEventMode(_event_mode);
 
  TLOG(TLVL_DEBUG) << "--- DONE, run plan:" << _run_plan_fn;

  return rc;
}


//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandleInit(const std::vector<std::string>& args) {

  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  fEqConfBuffer = "SYSTEM";
//-----------------------------------------------------------------------------
// cache the ODB handle, as need to loop over the keys in InitArtdaq
//-----------------------------------------------------------------------------
  HNDLE  hDB;                     // need to loop over ? 
  cm_get_experiment_database(&hDB, NULL);

  _odb_i                      = OdbInterface::Instance(hDB);
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  
  _event_mode                 = _odb_i->GetEventMode    (_h_active_run_conf);
  _h_cfo                      = _odb_i->GetCfoConfHandle(_h_active_run_conf);

  _enabled                    = _odb_i->GetEnabled      (_h_cfo);
  if (_enabled == 0) {
    TLOG(TLVL_ERROR) << "CFO disabled, return ERROR";
    return TMFeErrorMessage("CFO disabled, return ERROR"); 
  }

  std::string private_subnet  = _odb_i->GetPrivateSubnet(_h_active_run_conf);
  std::string public_subnet   = _odb_i->GetPublicSubnet (_h_active_run_conf);
  std::string active_run_conf = _odb_i->GetRunConfigName(_h_active_run_conf);
//-----------------------------------------------------------------------------
// ultiimately, these parameters should be common for 'emulated' and 'external' modes
//-----------------------------------------------------------------------------
  _emulated_mode              = _odb_i->GetCfoEmulatedMode      (_h_cfo);
  _n_ewm_train                = _odb_i->GetCfoNEventsPerTrain   (_h_cfo);
  _ew_length                  = _odb_i->GetEWLength             (_h_cfo);
  _first_ts                   = (uint64_t) _odb_i->GetFirstEWTag(_h_cfo);  // normally, start from zero
  _sleep_time_ms              = _odb_i->GetCfoSleepTime         (_h_cfo);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _host_label     = get_short_host_name(public_subnet.data());
  _full_host_name = get_full_host_name (private_subnet.data());

  _h_frontend_conf = _odb_i->GetFrontendConfHandle(_h_active_run_conf,_host_label);

  //  _odb_i->GetInteger(_h_frontend_conf,"Monitor/Dtc"   ,&_monitorDtc   );

  TLOG(TLVL_DEBUG) << "active_run_conf:"  << active_run_conf 
                   << " public_subnet:"   << public_subnet
                   << " private subnet:"  << private_subnet 
                   << " _full_host_name:" << _full_host_name
                   << " _host_label:"     << _host_label
                   << std::endl;
  EqSetStatus("Started...", "white");
  fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  int rc(0);
  if (_emulated_mode == 1) rc = InitEmulatedCfo();
  else                     rc = InitExternalCfo();

  if (rc == 0) return TMFeOk();
  else         return TMFeErrorMessage("failed to initialize the CFO"); 
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void TEquipmentCfo::HandlePeriodic() {

  TLOG(TLVL_DEBUG+1) << "--- START";

  if (_emulated_mode == 1) {
    
    int run_state(-1);

    _odb_i->GetInteger(0,"/Runinfo/State",&run_state);
    
    if ((run_state == STATE_RUNNING) and (_n_ewm_train > 0)) {
      _dtc_i->LaunchRunPlanEmulatedCfo(_ew_length,_n_ewm_train+1,_first_ts);
      _first_ts += _n_ewm_train;
    }
  }
  
  TLOG(TLVL_DEBUG+1) << "--- END";

  EqSetStatus(Form("OK"),"#00FF00");
}


//-----------------------------------------------------------------------------
// at begin rum, the CFO starts executing the run plan
// assume that from run to run the configuration can change
//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandleBeginRun(int RunNumber)  {

  TLOG(TLVL_DEBUG) << "--- START";
  
  _h_active_run_conf = _odb_i->GetActiveRunConfigHandle();

  if (_emulated_mode == 0) {
//-----------------------------------------------------------------------------
// in 'external' mode, [re-]initialize and start executing the run plan
//-----------------------------------------------------------------------------
    InitExternalCfo();
// external CFO starts executing its plan once at begin run
    _cfo_i->InitReadout(_run_plan_fn.data(),_link_mask);
    _cfo_i->LaunchRunPlan();
  }
  else {
//-----------------------------------------------------------------------------
// in emulated mode, at begin run perform only [re-]initialization,
// EWM's are sent by HandlePeriodic
//-----------------------------------------------------------------------------
    InitEmulatedCfo();
  }

  TLOG(TLVL_DEBUG) << "--- END";

  return TMFeOk();
};

//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentCfo::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}
