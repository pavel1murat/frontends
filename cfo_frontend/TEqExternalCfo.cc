//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "cfo_frontend/TEqExternalCfo.hh"
#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqExternalCfo"

//-----------------------------------------------------------------------------
TEqExternalCfo::TEqExternalCfo(const char* eqname, const char* eqfilename): TMu2eEqBase(eqname,eqfilename) {
  // fEqConfEventID          = 3;
  // fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  // fEqConfLogHistory       = 1;
  // fEqConfWriteEventsToOdb = true;

  _cfo_i                  = nullptr;
}

//-----------------------------------------------------------------------------
TEqExternalCfo::~TEqExternalCfo() {
}
//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEqExternalCfo::Init() {

  // fEqConfReadOnlyWhenRunning = false;
  // fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  //  fEqConfBuffer = "SYSTEM";
//-----------------------------------------------------------------------------
// cache the ODB handle, as need to loop over the keys in InitArtdaq
//-----------------------------------------------------------------------------
  _odb_i                      = OdbInterface::Instance();
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();


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

  //EqSetStatus("Started...", "white");
  //fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  int rc(0);
  // if (_emulated_mode == 1) rc = InitEmulatedCfo();
  // else                     rc = InitExternalCfo();

  if (rc == 0) return TMFeOk();
  else         return TMFeErrorMessage("failed to initialize the CFO"); 
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int TEqExternalCfo::HandlePeriodic() {
  int rc(0);

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

  //  EqSetStatus(Form("OK"),"#00FF00");

  return rc;
}


//-----------------------------------------------------------------------------
// at begin rum, the CFO starts executing the run plan
// assume that from run to run the configuration can change
//-----------------------------------------------------------------------------
int TEqExternalCfo::BeginRun(HNDLE H_RunConf)  {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "--- START";
  
  _h_active_run_conf = _odb_i->GetActiveRunConfigHandle();
//-----------------------------------------------------------------------------
// in 'external' mode, [re-]initialize and start executing the run plan
//-----------------------------------------------------------------------------
  // InitExternalCfo();
  // external CFO starts executing its plan once at begin run
  _cfo_i->InitReadout(_run_plan_fn.data(),_link_mask);
  _cfo_i->LaunchRunPlan();

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);

  return rc;
};

