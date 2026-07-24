//-----------------------------------------------------------------------------
// hKey points to '/Run'
// extract parameters from ODB, call corresponding function of the DTC
//-----------------------------------------------------------------------------

#include "utils/OdbInterface.hh"
#include "utils/TEquipmentManager.hh"
#include "node_frontend/TEqCrvDtc.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCrvDtc"

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread
// this function is static - MIDAS callback
//-----------------------------------------------------------------------------
void TEqCrvDtc::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();
//-----------------------------------------------------------------------------
// based on the key, figure out own name and the node name
// - this is the price paid for decoupling
//-----------------------------------------------------------------------------
  KEY k_cmd;                            // hKey corresponds to "Run"
  odb_i->GetKey(hKey,&k_cmd);

  HNDLE h_cmd = odb_i->GetParent(hKey);
  KEY k_dtc;
  odb_i->GetKey(h_cmd,&k_dtc);

  int pcie_addr(0);
  if (k_dtc.name[3] == '1') pcie_addr = 1;
//-----------------------------------------------------------------------------
// the command tree is assumed to have a form of .../mu2edaq09/DTC1/'
// so the frontend name is the same as the host label
//-----------------------------------------------------------------------------
  HNDLE h_frontend = odb_i->GetParent(h_cmd);
  KEY k_frontend;
  odb_i->GetKey(h_frontend,&k_frontend);
  
  TLOG(TLVL_DEBUG) << std::format("k_cmd.name:{} k_dtc.name:{} pcie_addr:{} k_frontend.name:{}",
                                  k_cmd.name,k_dtc.name,pcie_addr,k_frontend.name);
//-----------------------------------------------------------------------------
// get DTC config handle and check the DTC busy status
// if available, set it to BUSY (1)
// before issuing a new command, one has to check the DTC status
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm     = TEquipmentManager::Instance();
  std::string        eq_name = std::format("DTC{}",pcie_addr);
  TEqCrvDtc*         eq      = (TEqCrvDtc*) eqm->FindEquipmentItem(eq_name);

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(k_frontend.name,pcie_addr);
  int status  = odb_i->GetInteger(h_dtc,"Status");
  if (status != 0) {
    TLOG(TLVL_ERROR) << std::format("host:{} DTC:{} BUSY or in trouble",k_frontend.name,pcie_addr);
    // before returning, need to mark the command as finished to avoid duplicating the interlock
    odb_i->SetInteger(h_cmd,"Finished",1);
    return;
  }
    
  std::string cmd            = odb_i->GetString (h_cmd,"Name");
  std::string parameter_path = odb_i->GetString (h_cmd,"ParameterPath");
//-----------------------------------------------------------------------------

  TLOG(TLVL_DEBUG) << std::format("cmd:{} parameter_path:{}",cmd,parameter_path);

  int cmd_rc(0);
//-----------------------------------------------------------------------------
// clear status of the DTC and its ROCs
//------------------------------------------------------------------------------
  if      (cmd == "clear_status") {
    cmd_rc = eq->ClearStatus(h_cmd);
  }
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  else if      (cmd == "configure_ja") {
    cmd_rc = eq->ConfigureJA(h_cmd);
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
    std::thread t(&TEqCrvDtc::HardReset,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "init_readout") {
//-----------------------------------------------------------------------------
// init_readout
//-----------------------------------------------------------------------------
    std::thread t(&TEqCrvDtc::InitReadout,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// PRINT STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_status") {
    std::thread t(&TEqCrvDtc::PrintStatus,eq,h_cmd);
    t.detach();
  }  
//-----------------------------------------------------------------------------
// PRINT ROC STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_roc_status") {
    std::thread t(&TEqCrvDtc::PrintRocStatus,eq,h_cmd);
    t.detach();
  }  
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadRegister(h_cmd);
  }
  else if (cmd == "read_roc_register") {
//-----------------------------------------------------------------------------
// read ROC register
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadRocRegister(h_cmd);
  }
//-----------------------------------------------------------------------------
// READ_SUBEVENTS (interactive readout test)
//-----------------------------------------------------------------------------
  else if (cmd == "read_subevents") {
    std::thread t(&TEqCrvDtc::ReadSubevents,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "reset_output") {
                                        // eq 'Status' is handled in ResetOutput
    cmd_rc = eq->ResetOutput(h_cmd);
  }
  else if (cmd == "reset_roc") {
    cmd_rc = eq->ResetRoc(h_cmd);
  }
  else if (cmd == "soft_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    cmd_rc = eq->SoftReset(h_cmd);
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// WRITE_REGISTER
//-----------------------------------------------------------------------------
    cmd_rc = eq->WriteRegister(h_cmd);
  }
  else if (cmd == "write_roc_register") {
//-----------------------------------------------------------------------------
// WRITE_ROC_REGISTER
//-----------------------------------------------------------------------------
    cmd_rc = eq->WriteRocRegister(h_cmd);
  }
  else {
    cmd_rc = eq->UnknownCommand(h_cmd);
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order
// this printout shows up BEFORE the command output
//-----------------------------------------------------------------------------  
  TLOG(TLVL_DEBUG) << "-- END:" << " cmd_rc:" << cmd_rc;
}
