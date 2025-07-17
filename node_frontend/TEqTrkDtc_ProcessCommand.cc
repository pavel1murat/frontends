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
//
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
  // midas::odb o_dtc_cmd(dtc_cmd_buf_path);
                                        // should 0 or 1
  int run = odb_i->GetInteger(h_cmd,"Run");
  if (run == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }

  std::string cmd            = odb_i->GetString(h_cmd,"Name");
  std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
//-----------------------------------------------------------------------------
// this is address of the parameter record
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "cmd:" << cmd << " parameter_path:" << parameter_path;

  // midas::odb o_par(parameter_path);
  HNDLE h_par_path           = odb_i->GetHandle(0,parameter_path);
  TLOG(TLVL_DEBUG) << "-- parameters found";
//-----------------------------------------------------------------------------
// should be already defined at this point
//-----------------------------------------------------------------------------
  TEqTrkDtc*            eq_dtc = (TEqTrkDtc*) TEquipmentManager::Instance()->_eq_dtc[pcie_addr];
  trkdaq::DtcInterface* dtc_i  = eq_dtc->Dtc_i();
//-----------------------------------------------------------------------------
// PULSER_ON
//------------------------------------------------------------------------------
  if (cmd == "control_roc_pulser_on") {
    int link               = odb_i->GetInteger(h_par_path,"link"              );
    int first_channel_mask = odb_i->GetInteger(h_par_path,"first_channel_mask");
    int duty_cycle         = odb_i->GetInteger(h_par_path,"duty_cycle"        );
    int pulser_delay       = odb_i->GetInteger(h_par_path,"pulser_delay"      );

    TLOG(TLVL_DEBUG) << "link:" << link
                     << " first_channel_mask:" << first_channel_mask
                     << " duty_cycle:" << duty_cycle
                     << " pulser_delay:" << pulser_delay;
    try {
      dtc_i->ControlRoc_PulserOn(link,first_channel_mask,duty_cycle,pulser_delay);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute ControlRoc_PulserON ... BAIL OUT";
    }
  }
//-----------------------------------------------------------------------------
// PULSER_OFF
//------------------------------------------------------------------------------
  else if (cmd == "control_roc_pulser_off") {
    int link               = odb_i->GetInteger(h_par_path,"link"       );
    int print_level        = odb_i->GetInteger(h_par_path,"print_level");

    TLOG(TLVL_DEBUG) << "link:" << link << " print_level:" << print_level;
    try {
      dtc_i->ControlRoc_PulserOff(link,print_level);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "coudn't execute DtcInterface::ControlRoc_PulserOFF ... BAIL OUT";
    }
  }
//-----------------------------------------------------------------------------
// CONFIGURE_JA
//------------------------------------------------------------------------------
  else if (cmd == "configure_ja") {
    try {
      dtc_i->ConfigureJA();             // use defaults from the dtc_i settings
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
 
    eq_dtc->DumpSettings(ss);
  }
  else if (cmd == "dtc_control_roc_rates") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_RATES
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_rates";
 
    eq_dtc->Rates(ss);
  }
  else if (cmd == "dtc_control_roc_read") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_READ : link comes from ODB
//-----------------------------------------------------------------------------
    eq_dtc->Read(ss);
  }
  else if (cmd == "dtc_control_roc_read_ddr") {
    ss << std::endl;
    eq_dtc->ReadDDR(ss);
  }
  else if (cmd == "dtc_control_roc_digi_rw") {
//-----------------------------------------------------------------------------
// CONTROL_ROC_DIGI_RW
//-----------------------------------------------------------------------------
    ss << std::endl;  // ################ 

    eq_dtc->DigiRW(ss);
  }
  else if (cmd == "hard_reset") {
//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at hard reset";
 
    try         { dtc_i->Dtc()->HardReset(); ss << "hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "find_alignment") {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->FindAlignment(ss);
  }
  else if (cmd == "get_key") {
//-----------------------------------------------------------------------------
// GET KEY ... TODO
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->GetKey(ss);
  }
  else if (cmd == "get_roc_design_info") {
//-----------------------------------------------------------------------------
// get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->GetRocDesignInfo(ss);
  }
//-----------------------------------------------------------------------------
// LOAD_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "load_thresholds") {
    ss << std::endl;
    eq_dtc->LoadThresholds(ss);
  }
//-----------------------------------------------------------------------------
// MEASURE_THRESHOLDS
//-----------------------------------------------------------------------------
  else if (cmd == "measure_thresholds") {
    ss << std::endl;
    eq_dtc->MeasureThresholds(ss);
  }
//-----------------------------------------------------------------------------
// PRINT ROC STATUS
//-----------------------------------------------------------------------------
  else if (cmd == "print_roc_status") {
    ss << std::endl;
    try         { dtc_i->PrintRocStatus(1,-1,ss); }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
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
      eq_dtc->PrintRocStatus(ss);
    }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }


    
    try         { dtc_i->PrintStatus(ss); }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }  
  else if (cmd == "pulser_on") {
//-----------------------------------------------------------------------------
// PULSER_ON
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->ReadIlp(ss);
  }
  else if (cmd == "read_ilp") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->ReadIlp(ss);
  }
  else if (cmd == "read_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->ReadRegister(ss);
  }
  else if (cmd == "read_roc_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->ReadRocRegister(ss);
  }
  else if (cmd == "read_spi") {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->ReadSpi(ss);
  }
  else if (cmd == "reset_roc") {
    eq_dtc->ResetRoc(ss);
  }
  else if (cmd == "set_caldac") {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
 
     eq_dtc->SetCalDac(ss);
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
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
 
    try         { dtc_i->Dtc()->SoftReset(); ss << "soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (cmd == "write_register") {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->WriteRegister(ss);
  }
  else if (cmd == "write_roc_register") {
//-----------------------------------------------------------------------------
// WRITE_ROC_REGISTER
//-----------------------------------------------------------------------------
    ss << std::endl;
    eq_dtc->WriteRocRegister(ss);
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }
//-----------------------------------------------------------------------------
// need to send ss.str() to the appropriate log
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_cmd,"Finished",1);
  
  TLOG(TLVL_DEBUG) << "-- END";
}

