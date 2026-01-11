//-----------------------------------------------------------------------------
// hKey points to '/Run'
// extract parameters from ODB, call corresponding function of the DTC
//-----------------------------------------------------------------------------

#include "utils/OdbInterface.hh"
#include "node_frontend/TEquipmentManager.hh"
#include "node_frontend/TEqCrvDtc.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCrvDtc"

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread 
//-----------------------------------------------------------------------------
void TEqCrvDtc::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START TEqCrvDtc::" << __func__;

  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream ss;

  OdbInterface* odb_i = OdbInterface::Instance();
//-----------------------------------------------------------------------------
// based on the key, figure out own name and the node name
// - this is the price paid for decoupling
//-----------------------------------------------------------------------------
  KEY k;
  odb_i->GetKey(hKey,&k);

  HNDLE h_cmd = odb_i->GetParent(hKey);
  KEY dtc;
  odb_i->GetKey(h_cmd,&dtc);

  int pcie_addr(0);
  if (dtc.name[3] == '1') pcie_addr = 1;
//-----------------------------------------------------------------------------
// the command tree is assumed to have a form of .../mu2edaq09/DTC1/'
// so the frontend name is the same as the host label
//-----------------------------------------------------------------------------
  HNDLE h_frontend = odb_i->GetParent(h_cmd);
  KEY frontend;
  odb_i->GetKey(h_frontend,&frontend);
  
  TLOG(TLVL_DEBUG) << "k.name:" << k.name
                   << " dtc.name:" << dtc.name
                   << " pcie_addr:" << pcie_addr
                   << " frontend.name:" << frontend.name;

  //  std::string cmd_buf_path = std::format("/Mu2e/Commands/Frontends/{}/{}",frontend.name,dtc.name);

                                        // should 0 or 1
  int run = odb_i->GetInteger(h_cmd,"Run");
  if (run == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }
//-----------------------------------------------------------------------------
// get DTC config handle and set the DTC busy status
//-----------------------------------------------------------------------------
  HNDLE h_dtc = odb_i->GetDtcConfigHandle(frontend.name,pcie_addr);
  odb_i->SetInteger(h_dtc,"Status",1);
  
  std::string cmd            = odb_i->GetString (h_cmd,"Name");
  std::string parameter_path = odb_i->GetString (h_cmd,"ParameterPath");
  int link                   = odb_i->GetInteger(h_cmd,"link");
//-----------------------------------------------------------------------------
// this is address of the parameter record
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "cmd:" << cmd << " parameter_path:" << parameter_path;

  //  HNDLE h_par_path           = odb_i->GetHandle(0,parameter_path);
//-----------------------------------------------------------------------------
// should be already defined at this point
//-----------------------------------------------------------------------------
  TEqCrvDtc*             eq_dtc = (TEqCrvDtc*) TEquipmentManager::Instance()->_eq_dtc[pcie_addr];
  mu2edaq::DtcInterface* dtc_i  = eq_dtc->Dtc_i();

  ss << "--host_label:" << eq_dtc->HostLabel() << " host_name:" << eq_dtc->FullHostName()
     << " cmd:" << cmd << " pcie_addr:" << dtc_i->PcieAddr()
     << " link:" << link; // << " parameter_path:" << parameter_path;
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if      (cmd == "configure_ja") {
    try {
      cmd_rc = dtc_i->ConfigureJA();             // use defaults from the dtc_i settings
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute DtcInterface::ConfigureJA ... BAIL OUT";
    }
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
    // ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at hard_reset";
 
    try         { dtc_i->Dtc()->HardReset(); ss << " hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "init_readout") {
//-----------------------------------------------------------------------------
// init_readout
//-----------------------------------------------------------------------------
    // ss << std::endl;
    cmd_rc = eq_dtc->InitReadout(ss);
  }
//-----------------------------------------------------------------------------
// PRINT STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_status") {
    ss << std::endl;
    try         { dtc_i->PrintStatus(ss); }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }  
//-----------------------------------------------------------------------------
// PRINT ROC STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_roc_status") {
    ss << std::endl;
    try {
      cmd_rc = eq_dtc->PrintRocStatus(ss);
    }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  }  
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read register
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->ReadRegister(ss);
  }
  else if (cmd == "read_roc_register") {
//-----------------------------------------------------------------------------
// read ROC register
//-----------------------------------------------------------------------------
    cmd_rc = eq_dtc->ReadRocRegister(ss);
  }
  else if (cmd == "reset_output") {
    cmd_rc = eq_dtc->ResetOutput();
  }
  else if (cmd == "soft_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at soft_reset";
 
    try         { dtc_i->Dtc()->SoftReset(); ss << " soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->WriteRegister(ss);
  }
  else if (cmd == "write_roc_register") {
//-----------------------------------------------------------------------------
// WRITE_ROC_REGISTER
//-----------------------------------------------------------------------------
    cmd_rc = eq_dtc->WriteRocRegister(ss);
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order 
//-----------------------------------------------------------------------------
  cmd_rc = eq_dtc->WriteOutput(ss.str());
  
//-----------------------------------------------------------------------------
// done, avoid second call - leave "Run" = 1;, before setting it to 1 again,
// need to make sure that "Finished" = 1
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_cmd,"Finished",1);
//-----------------------------------------------------------------------------
// and set the DTC status
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_dtc,"Status",cmd_rc);
  
  TLOG(TLVL_DEBUG) << "-- END TEqCrvDtc::" << __func__ << " cmd_rc:" << cmd_rc;
}
