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

  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream ss;

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
  trkdaq::CfoInterface* cfo_i = eq->Cfo_i();

  ss << std::format("-- label:{} host:{} cmd:{} pcie_addr:{}",
                    eq->HostLabel(),eq->FullHostName(),cmd,cfo_i->PcieAddr());
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if      (cmd == "configure_ja") {
    try {
      eq->SetStatus(1);
      cmd_rc = cfo_i->ConfigureJA();             // use defaults from the dtc_i settings
      eq->SetStatus(cmd_rc);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute DtcInterface::ConfigureJA ... BAIL OUT";
    }
  }
//-----------------------------------------------------------------------------
// COMPILE_RUN_PLAN
//-----------------------------------------------------------------------------
  else if (cmd == "compile_run_plan") {
    std::thread t(&TEqHardwareCfo::CompileRunPlan,eq,h_cmd);
    t.detach();
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
    TLOG(TLVL_DEBUG) << "arrived at hard_reset";
 
    try         {
      eq->SetStatus(1);
      cfo_i->Cfo()->HardReset();
      eq->SetStatus(0);
      ss << " hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the CFO ... BAIL OUT" << std::endl; }
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
    ss << std::endl;
    try         {
      eq->SetStatus(1);
      cfo_i->PrintStatus(ss);
      eq->SetStatus(0);
    }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }  
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read register
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq->SetStatus(1);
    cmd_rc = eq->ReadRegister(h_cmd);
    eq->SetStatus(cmd_rc);
  }
  else if (cmd == "reset_output") {
                                        // figure which logfile to reset
    
    cmd_rc = eq->ResetOutput(logfile);
  }
  else if (cmd == "soft_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    try         {
      eq->SetStatus(1);
      cfo_i->Cfo()->SoftReset();
      eq->SetStatus(0);
      ss << " soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the CFO ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// WRITE_REGISTER
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq->SetStatus(1);
    cmd_rc = eq->WriteRegister(h_cmd);
    eq->SetStatus(cmd_rc);
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order 
//-----------------------------------------------------------------------------
  cmd_rc = eq->WriteOutput(ss.str(),logfile);
  
  TLOG(TLVL_DEBUG) << "-- END:" << " cmd_rc:" << cmd_rc;
}
