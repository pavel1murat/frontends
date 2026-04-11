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
  int         link           = odb_i->GetInteger(h_cmd,"link");
  std::string logfile        = odb_i->GetString (h_cmd,"logfile");
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm = TEquipmentManager::Instance();

  std::string           eq_name = std::format("DTC{}",pcie_addr);
  TEqTrkDtc*            eq      = (TEqTrkDtc*) eqm->FindEquipmentItem(eq_name);
  trkdaq::DtcInterface* dtc_i   = eq->Dtc_i();

  TLOG(TLVL_DEBUG) << std::format("cmd:{} parameter_path:{} logfile:{}",cmd,parameter_path,logfile);
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if      (cmd == "configure_ja") {
    cmd_rc = eq->ConfigureJA(h_cmd);
  }
//-----------------------------------------------------------------------------
// DUMP_SETTINGS
//-----------------------------------------------------------------------------
  else if (cmd == "dump_settings") {
    cmd_rc = eq->DumpSettings(h_cmd);
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
    std::thread t(&TEqTrkDtc::FindThresholds,eq,h_cmd);
    t.detach();
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
// fast  get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    cmd_rc = eq->GetRocDesignInfo(h_cmd);
  }
//-----------------------------------------------------------------------------
// GET KEY
//-----------------------------------------------------------------------------
  else if (cmd == "get_key") {
    cmd_rc = eq->GetKey(h_cmd);
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::HardReset,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "init_readout") {
//-----------------------------------------------------------------------------
// init_readout
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::InitReadout,eq,h_cmd);
    t.detach();
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
    std::thread t(&TEqTrkDtc::PrintStatus,eq,h_cmd);
    t.detach();
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
    cmd_rc = eq->PulserOff(h_cmd);
  }
  else if (cmd == "pulser_on") {
//-----------------------------------------------------------------------------
// PULSER_ON
//-----------------------------------------------------------------------------
    cmd_rc = eq->PulserOn(h_cmd);
  }
  else if (cmd == "rates") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_RATES
//-----------------------------------------------------------------------------
    cmd_rc = eq->Rates(h_cmd);
  }
  else if (cmd == "read") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_READ : link comes from ODB
//-----------------------------------------------------------------------------
    cmd_rc = eq->Read(h_cmd);
  }
  else if (cmd == "read_ddr") {
    cmd_rc = eq->ReadDDR(h_cmd);
  }
  else if (cmd == "read_device_id") {
//-----------------------------------------------------------------------------
// read device ID
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadDeviceID(h_cmd);
  }
  else if (cmd == "read_mnid") {
//-----------------------------------------------------------------------------
// read panel MinnesotaID
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadMnID(h_cmd);
  }
  else if (cmd == "read_ilp") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadIlp(h_cmd);
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
  else if (cmd == "read_spi") {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    cmd_rc = eq->ReadSpi(h_cmd);
  }
//-----------------------------------------------------------------------------
// READ_SUBEVENTS (interactive readout test)
//-----------------------------------------------------------------------------
  else if (cmd == "read_subevents") {
    std::thread t(&TEqTrkDtc::ReadSubevents,eq,h_cmd);
    t.detach();
  }
  else if (cmd == "reboot_mcu") {
    cmd_rc = eq->RebootMcu(h_cmd);
  }
  else if (cmd == "reset_digis") {
    cmd_rc = eq->ResetDigis(h_cmd);
  }
  else if (cmd == "reset_output") {
                                        // eq 'Status' is handled in ResetOutput
    cmd_rc = eq->ResetOutput(h_cmd);
  }
  else if (cmd == "reset_roc") {
    cmd_rc = eq->ResetRoc(h_cmd);
  }
//-----------------------------------------------------------------------------
// SET CALDAC (set height of the calibration pulse)
//-----------------------------------------------------------------------------
  else if (cmd == "set_caldac") {
     cmd_rc = eq->SetCalDac(h_cmd);
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
    cmd_rc = eq->SoftReset(h_cmd);
  }
  else if (cmd == "test_command") {
//-----------------------------------------------------------------------------
// test command; for different DTCs may be executed in a thread, links are processes sequentially
// what starts the thread ??  - this is open so far
//-----------------------------------------------------------------------------
    std::thread t(&TEqTrkDtc::TestCommand,eq,h_cmd);
    t.detach();
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
