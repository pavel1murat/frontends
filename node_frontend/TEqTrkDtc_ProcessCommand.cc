//-----------------------------------------------------------------------------
// hKey points to '/Run'
// extract parameters from ODB, call corresponding function of the DTC
//-----------------------------------------------------------------------------

#include "utils/OdbInterface.hh"
#include "utils/TEquipmentManager.hh"
#include "node_frontend/TEqTrkDtc.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread
// this function is static - MIDAS callback
//-----------------------------------------------------------------------------
void TEqTrkDtc::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START";

  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream ss;

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
  HNDLE h_dtc = odb_i->GetDtcConfigHandle(k_frontend.name,pcie_addr);
  int status = odb_i->GetInteger(h_dtc,"Status");
  if (status != 0) {
    TLOG(TLVL_ERROR) << std::format("host:{} DTC:{} BUSY or in trouble",k_frontend.name,pcie_addr);
    return;
  }
    
  std::string cmd            = odb_i->GetString (h_cmd,"Name");
  std::string parameter_path = odb_i->GetString (h_cmd,"ParameterPath");
  int link                   = odb_i->GetInteger(h_cmd,"link");
  std::string logfile        = odb_i->GetString (h_cmd,"logfile");
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm = TEquipmentManager::Instance();

  std::string eq_name = std::format("DTC{}",pcie_addr);
  TEqTrkDtc*            eq = (TEqTrkDtc*) eqm->FindEquipmentItem(eq_name);
  trkdaq::DtcInterface* dtc_i  = eq->Dtc_i();

  ss << std::format("-- label:{} host:{} cmd:{} pcie_addr:{} link:{}",
                    eq->HostLabel(),eq->FullHostName(),cmd,dtc_i->PcieAddr(),link);

  TLOG(TLVL_DEBUG) << std::format("cmd:{} parameter_path:{} logfile:{}",cmd,parameter_path,logfile);
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if      (cmd == "configure_ja") {
    try {
      odb_i->SetInteger(h_dtc,"Status",1);
      cmd_rc = dtc_i->ConfigureJA();             // use defaults from the dtc_i settings
      odb_i->SetInteger(h_dtc,"Status",0);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute DtcInterface::ConfigureJA ... BAIL OUT";
    }
  }
//-----------------------------------------------------------------------------
// DUMP_SETTINGS
//-----------------------------------------------------------------------------
  else if (cmd == "dump_settings") {
    ss << std::endl;
    TLOG(TLVL_DEBUG) << std::format("arrived at {}",cmd);
 
    cmd_rc = eq->DumpSettings(ss);
  }
//-----------------------------------------------------------------------------
// FIND_ALIGNMENT
//-----------------------------------------------------------------------------
  else if (cmd == "find_alignment") {
    std::thread t(&TEqTrkDtc::FindAlignment,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// FIND_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "find_thresholds") {
    ss << std::endl;
    cmd_rc = eq->FindThresholds(ss);
  }
  else if (cmd == "digi_read") {
//-----------------------------------------------------------------------------
// DIGI_READ
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::DigiRead,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "digi_rw") {
//-----------------------------------------------------------------------------
// DIGI_RW
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::DigiRW,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "digi_write") {
//-----------------------------------------------------------------------------
// DIGI_WRITE
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::DigiWrite,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "get_design_info") {
//-----------------------------------------------------------------------------
// get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->GetRocDesignInfo(ss);
  }
//-----------------------------------------------------------------------------
// GET KEY
//-----------------------------------------------------------------------------
  else if (cmd == "get_key") {
    ss << std::endl;
    cmd_rc = eq->GetKey(ss);
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
    // ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at hard_reset";
 
    try         {
      odb_i->SetInteger(h_dtc,"Status",1);
      dtc_i->Dtc()->HardReset();
      odb_i->SetInteger(h_dtc,"Status",0);
      ss << " hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "init_readout") {
//-----------------------------------------------------------------------------
// init_readout
//-----------------------------------------------------------------------------
    cmd_rc = eq->InitReadout(ss);
  }
//-----------------------------------------------------------------------------
// LOAD_THRESHOLDS - execute per-DTC commands as threads
//-----------------------------------------------------------------------------
  else if (cmd == "load_thresholds") {
    std::thread t(&TEqTrkDtc::LoadThresholds,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// MEASURE_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "measure_thresholds") {
    std::thread t(&TEqTrkDtc::MeasureThresholds,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// PRINT STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_status") {
    ss << std::endl;
    try         {
      odb_i->SetInteger(h_dtc,"Status",1);
      dtc_i->PrintStatus(ss);
      odb_i->SetInteger(h_dtc,"Status",0);
    }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }  
//-----------------------------------------------------------------------------
// PRINT ROC STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_roc_status") {
    std::thread t(&TEqTrkDtc::PrintRocStatus,eq,h_cmd);
    t.detach();
  }  
  else if (cmd == "pulser_off") {
//-----------------------------------------------------------------------------
// PULSER_OFF
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->PulserOff(ss);
  }
  else if (cmd == "pulser_on") {
//-----------------------------------------------------------------------------
// PULSER_ON
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->PulserOn(ss);
  }
  else if (cmd == "rates") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_RATES
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at rates";
 
    cmd_rc = eq->Rates(ss);
  }
  else if (cmd == "read") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_READ : link comes from ODB
//-----------------------------------------------------------------------------
    eq->Read(ss);
  }
  else if (cmd == "read_ddr") {
    ss << std::endl;
    cmd_rc = eq->ReadDDR(ss);
  }
  else if (cmd == "read_device_id") {
//-----------------------------------------------------------------------------
// read device ID
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->ReadDeviceID(ss);
  }
  else if (cmd == "read_mnid") {
//-----------------------------------------------------------------------------
// read panel MinnesotaID
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->ReadMnID(ss);
  }
  else if (cmd == "read_ilp") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->ReadIlp(ss);
  }
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->ReadRegister(ss);
  }
  else if (cmd == "read_roc_register") {
//-----------------------------------------------------------------------------
// read ROC register
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadRocRegister(ss);
  }
  else if (cmd == "read_spi") {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->ReadSpi(ss);
  }
//-----------------------------------------------------------------------------
// READ_SUBEVENTS (interactive readout test)
//-----------------------------------------------------------------------------
  else if (cmd == "read_subevents") {
    std::thread t(&TEqTrkDtc::ReadSubevents,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "reboot_mcu") {
    cmd_rc = eq->RebootMcu(ss);
  }
  else if (cmd == "reset_output") {
    cmd_rc = eq->ResetOutput(logfile);
  }
  else if (cmd == "reset_digis") {
    cmd_rc = eq->ResetDigis(ss);
  }
  else if (cmd == "reset_roc") {
    cmd_rc = eq->ResetRoc(ss);
  }
//-----------------------------------------------------------------------------
// RESET ROC
//-----------------------------------------------------------------------------
  else if (cmd == "set_caldac") {
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
 
     cmd_rc = eq->SetCalDac(ss);
  }
//-----------------------------------------------------------------------------
// SET ROC DELAY(s)
//-----------------------------------------------------------------------------
  else if (cmd == "set_roc_delays") {
    std::thread t(&TEqTrkDtc::SetRocDelay,eq,h_cmd);
    t.detach();
  }
//-----------------------------------------------------------------------------
// LOASET_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "set_thresholds") {
    std::thread t(&TEqTrkDtc::SetThresholds,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "soft_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at soft_reset";
 
    try         {
      odb_i->SetInteger(h_dtc,"Status",1);
      dtc_i->Dtc()->SoftReset();
      odb_i->SetInteger(h_dtc,"Status",0);
      ss << " soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "test_command") {
//-----------------------------------------------------------------------------
// test command; for different DTCs may be executed in a thread, links are processes sequentially
// what starts the thread ??  - this is open so far
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->TestCommand(ss);
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// WRITE_REGISTER
//-----------------------------------------------------------------------------
    ss << std::endl;
    cmd_rc = eq->WriteRegister(ss);
  }
  else if (cmd == "write_roc_register") {
//-----------------------------------------------------------------------------
// WRITE_ROC_REGISTER
//-----------------------------------------------------------------------------
    cmd_rc = eq->WriteRocRegister(ss);
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order
// this printout shows up BEFORE the command output
//-----------------------------------------------------------------------------
  cmd_rc = eq->WriteOutput(ss.str(),logfile);
  
  TLOG(TLVL_DEBUG) << "-- END:" << " cmd_rc:" << cmd_rc;
}
