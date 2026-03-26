//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "cfo_frontend/TEqHardwareCfo.hh"
#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqHardwareCfo"

//-----------------------------------------------------------------------------
TEqHardwareCfo::TEqHardwareCfo(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Cfo):
  TMu2eEqBase(Name,Title,TMu2eEqBase::kDaq) {
  // fEqConfEventID          = 3;
  // fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  // fEqConfLogHistory       = 1;
  // fEqConfWriteEventsToOdb = true;

  TLOG(TLVL_DEBUG) << "-- START";
  
  _handle = H_Cfo;
  _cfo_i  = nullptr;
  Init();
  
  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
TEqHardwareCfo::~TEqHardwareCfo() {
}
//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEqHardwareCfo::Init() {

  // fEqConfReadOnlyWhenRunning = false;
  // fEqConfWriteEventsToOdb    = true;
  // fEqConfLogHistory = 1;

  //  fEqConfBuffer = "SYSTEM";
//---------------------------------------------------------------------------
// in principle, can compile the run plan on the fly, make it a next step
// CfoGetRunPlan returns a string - filename of the compiled run plan file 
//---------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "--- START";
//-----------------------------------------------------------------------------
// hardware CFO is a separate hardware unit, and that can't be enabled if
// the CFO is disabled in the run configuration
// an emulated CFO configuration includs a link to the DTC
//-----------------------------------------------------------------------------
  // _event_mode            = _odb_i->GetEventMode(_h_active_run_conf);
  _pcie_addr             = _odb_i->GetInteger(_handle,"pcie_addr"); // needed by the boardreader
   int timing_chain_mask = _odb_i->GetUInt32 (_handle,"timing_chain_mask");
   int event_mode        = _odb_i->GetInteger(_handle,"event_mode");
  
  TLOG(TLVL_DEBUG) << std::format("event_mode:{} pcie_addr:{} timing_chain_mask:0x{:08x}",event_mode,_pcie_addr,timing_chain_mask);

  _cfo_i             = trkdaq::CfoInterface::Instance(_pcie_addr,timing_chain_mask);
  _cfo_i->fJAMode    = _odb_i->GetJAMode   (_handle);
  _cfo_i->SetEventMode(event_mode);
 
  // TLOG(TLVL_DEBUG) << "--- DONE, run plan:" << _run_plan;
  
  int enabled  = _odb_i->GetEnabled(_handle);
  if (enabled == 0) {
    std::string msg("CFO disabled, return ERROR");
    TLOG(TLVL_ERROR) << msg;
    return TMFeErrorMessage(msg); 
  }
//-----------------------------------------------------------------------------
// hotlinks - start from one function handling both DTCs
// command processor : 'ProcessCommand' function
//-----------------------------------------------------------------------------
    HNDLE hdb       = _odb_i->GetDbHandle();
    HNDLE h_cmd     = _odb_i->GetCfoCmdHandle(_h_active_run_conf);
    HNDLE h_cmd_run = _odb_i->GetHandle(h_cmd,"Run");

    TLOG(TLVL_DEBUG) << std::format("before db_open_record: h_cmd_run:{} _cmd_run:{}",h_cmd_run,_cmd_run);
    
    if (db_open_record(hdb,h_cmd_run,&_cmd_run,sizeof(int32_t),MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
      std::string msg = std::format("cannot open CFO hotlink in ODB");
      cm_msg(MERROR, __func__,msg.data());
      TLOG(TLVL_ERROR) << msg;
    }

  //EqSetStatus("Started...", "white");
  //fMfe->Msg(MINFO, "HandleInit", std::format("Init {}","+ Ok!").data());

  int rc(0);

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  
  if (rc == 0) return TMFeOk();
  else         return TMFeErrorMessage("failed to initialize the CFO"); 
}

//-----------------------------------------------------------------------------
// HW CFO : do nothing
//-----------------------------------------------------------------------------
int TEqHardwareCfo::HandlePeriodic() {
  int rc(0);

  TLOG(TLVL_DEBUG+1) << "--- START";
  TLOG(TLVL_DEBUG+1) << "--- END";

  //  EqSetStatus(Form("OK"),"#00FF00");

  return rc;
}


//-----------------------------------------------------------------------------
// at begin rum, the CFO starts executing the run plan
// assume that from run to run the configuration can change
//-----------------------------------------------------------------------------
int TEqHardwareCfo::BeginRun(int RunNumber)  {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: run_number:{}",RunNumber);
//-----------------------------------------------------------------------------
// in 'external' mode, [re-]initialize and start executing the run plan
//
//-----------------------------------------------------------------------------
  std::string run_plan_dir      = _odb_i->GetCfoRunPlanDir();
  std::string run_plan          = _odb_i->GetCfoRunPlan(_handle);
  std::string run_plan_fn       = run_plan_dir+'/'+run_plan+".bin";
  int         timing_chain_mask = _odb_i->GetUInt32(_handle,"timing_chain_mask");  // timing_chain_mask is a UINT32

  TLOG(TLVL_DEBUG) << std::format("run_plan_dir:{} run_plan:{} run_plan_fn:{}",run_plan_dir,run_plan,run_plan_fn);

  _cfo_i->InitReadout(run_plan_fn,timing_chain_mask);
  _cfo_i->LaunchRunPlan();

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);

  return rc;
};

//-----------------------------------------------------------------------------
int TEqHardwareCfo::EndRun(int RunNumber)  {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: run_number:{}",RunNumber);
//-----------------------------------------------------------------------------
// in 'external' mode, [re-]initialize and start executing the run plan
//-----------------------------------------------------------------------------
  _cfo_i->Halt();

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);

  return rc;
};

