//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "cfo_frontend/TEqTimeChainManager.hh"
#include "utils/utils.hh"
#include "TString.h"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTimeChainManager"

//-----------------------------------------------------------------------------
TEqTimeChainManager::TEqTimeChainManager(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Cfo):
  TMu2eEqBase(Name,Title,TMu2eEqBase::kDaq) {
  // fEqConfEventID          = 3;
  // fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  // fEqConfLogHistory       = 1;
  // fEqConfWriteEventsToOdb = true;

  TLOG(TLVL_DEBUG) << "-- START";
  
  _handle = H_Cfo;
  //  _cfo_i  = nullptr;
  Init();
  
  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
TEqTimeChainManager::~TEqTimeChainManager() {
}
//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEqTimeChainManager::Init() {

  // fEqConfReadOnlyWhenRunning = false;
  // fEqConfWriteEventsToOdb    = true;
  // fEqConfLogHistory = 1;

  //  fEqConfBuffer = "SYSTEM";
//---------------------------------------------------------------------------
// in principle, can compile the run plan on the fly, make it a next step
// CfoGetRunPlan returns a string - filename of the compiled run plan file 
//---------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "-- START";
//-----------------------------------------------------------------------------
// hardware CFO is a separate hardware unit, and that can't be enabled if
// the CFO is disabled in the run configuration
// an emulated CFO configuration includs a link to the DTC
//-----------------------------------------------------------------------------
  // _pcie_addr            = _odb_i->GetInteger(_handle,"pcie_addr"); // needed by the boardreader
  // int timing_chain_mask = _odb_i->GetUInt32 (_handle,"timing_chain_mask");
  // int event_mode        = _odb_i->GetInteger(_handle,"event_mode");
  // int ja_mode           = _odb_i->GetInteger(_handle,"ja_mode");
  
  //  TLOG(TLVL_DEBUG) << std::format("event_mode:{} pcie_addr:{} timing_chain_mask:0x{:08x}",event_mode,_pcie_addr,timing_chain_mask);

  // _cfo_i                 = trkdaq::CfoInterface::Instance(_pcie_addr,timing_chain_mask);
  
  // _cfo_i->fJAMode        = ja_mode;
  // _cfo_i->SetEventMode(event_mode);
 
  // int enabled  = _odb_i->GetEnabled(_handle);
  // if (enabled == 0) {
  //   std::string msg("CFO disabled, return ERROR");
  //   TLOG(TLVL_ERROR) << msg;
  //   return TMFeErrorMessage(msg); 
  // }
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
int TEqTimeChainManager::HandlePeriodic() {
  int rc(0);

  TLOG(TLVL_DEBUG+1) << "-- START";
  TLOG(TLVL_DEBUG+1) << "-- END";

  //  EqSetStatus(Form("OK"),"#00FF00");

  return rc;
}

//-----------------------------------------------------------------------------
// at begin rum, the CFO starts executing the run plan
// assume that from run to run the configuration can change
//-----------------------------------------------------------------------------
int TEqTimeChainManager::BeginRun(int RunNumber)  {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: run_number:{}",RunNumber);
//-----------------------------------------------------------------------------
// in 'external' mode, [re-]initialize and start executing the run plan
//-----------------------------------------------------------------------------
  // std::string run_plan_dir      = _odb_i->GetCfoRunPlanDir();
  // std::string run_plan          = _odb_i->GetCfoRunPlan(_handle);
  // std::string run_plan_fn       = run_plan_dir+'/'+run_plan+".bin";

  
  // execute CFO 'init_readout' command

  midas::odb  cfo_command ("/Mu2e/Commands/DAQ/CFO");
  
  cfo_command["Name"         ] = "init_readout";
  cfo_command["ParameterPath"] = "/Mu2e/Commands/DAQ/CFO/init_readout";
  cfo_command["logfile"      ] = "mu2e-calo-13-cfo1";
  cfo_command["Finished"     ] = 0;
  cfo_command["Run"          ] = 1;

  // wait for completion, the wait should not break the bank

  int max_wait_time_ms(5000);
  int wait_time_ms(0);

  using clock = std::chrono::steady_clock;

  auto t0 = clock::now();

  while (wait_time_ms < max_wait_time_ms) {
    int finished = cfo_command["Finished"];
    if (finished == 1) {
      rc = cfo_command["ReturnCode"];
      break;
    }
    cm_yield(1000);

    auto t1      = clock::now();
    wait_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  }

  if (rc == 0) {                        // check timeout
    if (wait_time_ms >= max_wait_time_ms) {
      TLOG(TLVL_ERROR) << std::format("CFO initialization timed out after {} ms",max_wait_time_ms);
      rc = -1;
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- END: wait_time:{} max_wait_time:{} rc:{}",wait_time_ms, max_wait_time_ms,rc);

  return rc;
};

//-----------------------------------------------------------------------------
// for now , the run number is not actually used, can fake for testing
// faking means calling ::EndRun interactively
//-----------------------------------------------------------------------------
int TEqTimeChainManager::EndRun(int RunNumber)  {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: run_number:{}",RunNumber);
  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);

  return rc;
};

//-----------------------------------------------------------------------------
void TEqTimeChainManager::ProcessCommand(int hDB, int hKey, void* Info) {
  int rc(0);
  TLOG(TLVL_DEBUG) << "-- START:";
  TLOG(TLVL_DEBUG) << "-- END:";
}


//-----------------------------------------------------------------------------
// for single-link commands aim for a 1 line output,
// tracker always has station, plane, panel
//-----------------------------------------------------------------------------
int TEqTimeChainManager::StartMessage(HNDLE H_Cmd, std::stringstream& Stream) {

  std::string cmd_name = _odb_i->GetString (H_Cmd,"Name"   );
  // int         station  = _odb_i->GetInteger(H_Cmd,"station");
  // int         plane    = _odb_i->GetInteger(H_Cmd,"plane"  );
  // int         mnid     = _odb_i->GetInteger(H_Cmd,"mnid"   );

  auto now = std::chrono::system_clock::now();

  // {:%Y-%m-%d %H:%M:%S} uses standard strftime-style flags
  std::string s_now = std::format("{:%Y-%m-%d %H:%M:%S}", now);

  Stream << std::format("{} -- TEqTimeChainManager: cmd:{} WHY IS IT CALLED\n", s_now,cmd_name);
  return 0;
}

