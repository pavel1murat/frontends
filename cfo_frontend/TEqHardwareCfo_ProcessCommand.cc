//-----------------------------------------------------------------------------
// hKey points to '/Run'
// extract parameters from ODB, call corresponding function of the DTC
//-----------------------------------------------------------------------------

#include "utils/OdbInterface.hh"
#include "utils/TEquipmentManager.hh"
#include "cfo_frontend/TEqHardwareCfo.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqHardwareCfo"

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread
// this function is static...
//-----------------------------------------------------------------------------
void TEqHardwareCfo::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();
//-----------------------------------------------------------------------------
// based on the key, figure out own name and the node name
// - this is the price paid for decoupling
//-----------------------------------------------------------------------------
  KEY k_cmd;                            // hKey corresponds to "Run", h_cmd is its parent's handle
  odb_i->GetKey(hKey,&k_cmd);

  HNDLE h_cmd = odb_i->GetParent(hKey); // points to the CFO command record
//-----------------------------------------------------------------------------
// the CFO command tree: /Mu2e/Commands/DAQ/CFO, check if previous command has finished
//-----------------------------------------------------------------------------
  int finished = odb_i->GetInteger(h_cmd,"Finished");
  if (finished != 0) {
    std::string msg = "previous CFO command not finished";
    TLOG(TLVL_ERROR) << msg;
    cm_msg(MERROR, __func__,msg.data());
    cm_msg_flush_buffer();
    return;
  }

  std::string cmd            = odb_i->GetString (h_cmd,"Name");
  std::string parameter_path = odb_i->GetString (h_cmd,"ParameterPath");
  std::string logfile        = odb_i->GetString (h_cmd,"logfile");
//-----------------------------------------------------------------------------
// 'ParameterPath' defines the address of the actual command parameter record
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << std::format("cmd:{} parameter_path:{} logfile:{}",cmd,parameter_path,logfile);
//-----------------------------------------------------------------------------
// should be already defined at this point
//-----------------------------------------------------------------------------
  TEquipmentManager*    eqm   = TEquipmentManager::Instance();
  TEqHardwareCfo*       eq    = (TEqHardwareCfo*) eqm->FindEquipmentItem("CFO");
  //  trkdaq::CfoInterface* cfo_i = eq->Cfo_i();

//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if      (cmd == "configure_ja") {
    cmd_rc = eq->ConfigureJA(h_cmd);             // use defaults from the dtc_i settings
  }
//-----------------------------------------------------------------------------
// COMPILE_RUN_PLAN
//-----------------------------------------------------------------------------
  else if (cmd == "compile_run_plan") {
    std::thread t(&TEqHardwareCfo::CompileRunPlan,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// END_RUN: prototype
//-----------------------------------------------------------------------------
  else if (cmd == "end_run") {
    eq->EndRun(-1);
  }
//-----------------------------------------------------------------------------
// HALT
//-----------------------------------------------------------------------------
  else if (cmd == "halt") {
    std::thread t(&TEqHardwareCfo::Halt,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
  else if (cmd == "hard_reset") {
    eq->HardReset(h_cmd);
  }
//-----------------------------------------------------------------------------
// init_readout
//-----------------------------------------------------------------------------
  else if (cmd == "init_readout") {
    std::thread t(&TEqHardwareCfo::InitReadout,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// LAUNCH_RUN_PLAN
//-----------------------------------------------------------------------------
  else if (cmd == "launch_run_plan") {
    std::thread t(&TEqHardwareCfo::LaunchRunPlan,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// PRINT STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_status") {
    cmd_rc = eq->PrintStatus(h_cmd);
  }  
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read register
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadRegister(h_cmd);
  }
  else if (cmd == "reset_output") {
    cmd_rc = eq->ResetOutput(h_cmd);
  }
//-----------------------------------------------------------------------------
// SET_RUN_PLAN
//-----------------------------------------------------------------------------
  else if (cmd == "set_run_plan") {
    std::thread t(&TEqHardwareCfo::SetRunPlan,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "soft_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    eq->SoftReset(h_cmd);
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// WRITE_REGISTER
//-----------------------------------------------------------------------------
    cmd_rc = eq->WriteRegister(h_cmd);
  }
  else {
    eq->UnknownCommand(h_cmd);
  }
  
  TLOG(TLVL_DEBUG) << "-- END:" << " cmd_rc:" << cmd_rc;
}
