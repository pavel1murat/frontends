//-----------------------------------------------------------------------------
// hKey points to '/Run'
// extract parameters from ODB, call corresponding function of the DTC
//-----------------------------------------------------------------------------

#include "utils/OdbInterface.hh"
#include "node_frontend/TEquipmentManager.hh"
#include "node_frontend/TEqTrkDtc.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread 
//-----------------------------------------------------------------------------
void TEqTrkDtc::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START TEqTrkDtc::" << __func__;

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

  std::string cmd_buf_path = std::format("/Mu2e/Commands/Frontends/{}/{}",frontend.name,dtc.name);

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
  TEqTrkDtc*            eq_dtc = (TEqTrkDtc*) TEquipmentManager::Instance()->_eq_dtc[pcie_addr];
  trkdaq::DtcInterface* dtc_i  = eq_dtc->Dtc_i();

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
  else if (cmd == "dump_settings") {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dump_settings";
 
    cmd_rc = eq_dtc->DumpSettings(ss);
  }
  else if (cmd == "find_alignment") {
    cmd_rc = eq_dtc->FindAlignment(ss);
  }
  else if (cmd == "find_thresholds") {
//-----------------------------------------------------------------------------
// FIND_THRESHOLDS
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->FindThresholds(ss);
  }
  else if (cmd == "get_key") {
//-----------------------------------------------------------------------------
// GET KEY ... TODO
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->GetKey(ss);
  }
  else if (cmd == "get_roc_design_info") {
//-----------------------------------------------------------------------------
// get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->GetRocDesignInfo(ss);
  }
  else if (cmd == "digi_rw") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_DIGI_RW
//-----------------------------------------------------------------------------
    ss << std::endl;  // ################ 

    cmd_rc = eq_dtc->DigiRW(ss);
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
    // ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at hard_reset";
 
    try         { dtc_i->Dtc()->HardReset(); ss << "hart reset OK" << std::endl; }
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
// LOAD_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "load_thresholds") {
    ss << std::endl;
    cmd_rc = eq_dtc->LoadThresholds(ss);
  }
//-----------------------------------------------------------------------------
// MEASURE_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "measure_thresholds") {
    ss << std::endl;
    cmd_rc = eq_dtc->MeasureThresholds(ss);
  }
//-----------------------------------------------------------------------------
// // PRINT ROC STATUS
// //-----------------------------------------------------------------------------
//   else if (cmd == "print_roc_status") {
//     ss << std::endl;
//     try         { dtc_i->PrintRocStatus(1,-1,ss); }
//     catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
//   }
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
  else if (cmd == "pulser_off") {
//-----------------------------------------------------------------------------
// PULSER_OFF
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->PulserOff(ss);
  }
  else if (cmd == "program_roc") {
//-----------------------------------------------------------------------------
// PROGRAM_ROC
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->ProgramRoc(ss);
  }
  else if (cmd == "pulser_on") {
//-----------------------------------------------------------------------------
// PULSER_ON
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->PulserOn(ss);
  }
  else if (cmd == "rates") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_RATES
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at rates";
 
    cmd_rc = eq_dtc->Rates(ss);
  }
  else if (cmd == "read") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_READ : link comes from ODB
//-----------------------------------------------------------------------------
    eq_dtc->Read(ss);
  }
  else if (cmd == "read_ddr") {
    ss << std::endl;
    cmd_rc = eq_dtc->ReadDDR(ss);
  }
  else if (cmd == "read_ddr") {
//-----------------------------------------------------------------------------
// read DDR
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->ReadDDR(ss);
  }
  else if (cmd == "read_ilp") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->ReadIlp(ss);
  }
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read ILP
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
  else if (cmd == "read_spi") {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq_dtc->ReadSpi(ss);
  }
  else if (cmd == "reboot_mcu") {
    cmd_rc = eq_dtc->RebootMcu(ss);
  }
  else if (cmd == "reset_output") {
    cmd_rc = eq_dtc->ResetOutput();
  }
  else if (cmd == "reset_roc") {
    cmd_rc = eq_dtc->ResetRoc(ss);
  }
//-----------------------------------------------------------------------------
// RESET ROC
//-----------------------------------------------------------------------------
  else if (cmd == "set_caldac") {
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
 
     cmd_rc = eq_dtc->SetCalDac(ss);
  }
//-----------------------------------------------------------------------------
// LOASET_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "set_thresholds") {
    ss << std::endl;
    eq_dtc->SetThresholds(ss);
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
  
  TLOG(TLVL_DEBUG) << "-- END TEqTrkDtc::" << __func__ << " cmd_rc:" << cmd_rc;
}
