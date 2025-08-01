/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TTrkEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

using json = nlohmann::json;

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TTrkEquipmentNode"
//-----------------------------------------------------------------------------
namespace ns {

  struct parameters {
    int pcie;
  };

  void to_json(json& j, const parameters& p) {
    j = json{ {"pcie", p.pcie} };
  }
  
  void from_json(const json& j, parameters& p) {
    j.at("pcie").get_to(p.pcie);
  }
} // namespace ns

//-----------------------------------------------------------------------------
TMFeResult TTrkEquipmentNode::HandleRpc(const char* cmd, const char* args, std::string& response) {
  fMfe->Msg(MINFO, "HandleRpc", "RPC cmd [%s], args [%s]", cmd, args);

  std::stringstream& ss = fSSthr;

  OdbInterface* odb_i      = OdbInterface::Instance();
  HNDLE         h_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   conf_name  = odb_i->GetRunConfigName(h_run_conf);

  std::string conf_path("/Mu2e/RunConfigurations/");
  conf_path += conf_name;

  TLOG(TLVL_DEBUG) << "-- START: RPC cmd:" << cmd << " args:" << args << " conf_name:" << conf_name;
//-----------------------------------------------------------------------------
// in principle, string could contain a list of parameters to be parsed
// so far : only PCIE address
// so parse parametrers : expect a string in JSON format, like '{"pcie":1}' 
//-----------------------------------------------------------------------------
  int pcie_addr(-1), roc(-1);
  try {
    json j1 = json::parse(args);
    TLOG(TLVL_DEBUG) << j1 << " pcie:" << j1.at("pcie") << " roc:" << j1.at("roc") << std::endl;
    pcie_addr = j1.at("pcie");
    roc       = j1.at("roc" );
  }
  catch(...){
    std::string msg("couldn't parse json:");
    TLOG(TLVL_ERROR) << msg << args;
    return TMFeErrorMessage(msg+args);
  }

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,pcie_addr);
                                                     
  TLOG(TLVL_DEBUG) << "pcie_addr:" << pcie_addr
                   << " h_dtc:" << h_dtc
                   << " DTC[0]:" << fDtc_i[0]
                   << " DTC[1]:" << fDtc_i[1] ;
//-----------------------------------------------------------------------------
// start forming response
//-----------------------------------------------------------------------------
  response  = "cmd:";
  response += cmd;
  response += " args:";
  response += args;
//-----------------------------------------------------------------------------
// a single place to make sure that the pointer to the DTC is OK
//-----------------------------------------------------------------------------
  trkdaq::DtcInterface* dtc_i(nullptr);
  try {
    dtc_i = dynamic_cast<trkdaq::DtcInterface*>(fDtc_i[pcie_addr]);
  }
  catch(...) {
    TLOG(TLVL_ERROR) << "failed to initialize the DTC";
    ss << " failed to initialize the DTC" << pcie_addr ;
  }
  
  if (dtc_i == nullptr) {
    ss << " DTC" << pcie_addr << " is not enabled";
    response += ss.str();
    TLOG(TLVL_ERROR) << response;
    return TMFeErrorMessage(response);
  }

  if (strcmp(cmd,"configure_ja") == 0) {
//-----------------------------------------------------------------------------
// configure the jitter attenuator. Use dtc_i->fJAMode
//-----------------------------------------------------------------------------
//    HNDLE h_cmd        = odb_i->GetHandle(0,"/Mu2e/Commands/Tracker/DTC/configure_ja");

    int rc = dtc_i->ConfigureJA();
    if (rc == 0) ss << " SUCCESS";
    else         ss << " ERROR: failed to configure the JA";
  }
  else if (strcmp(cmd,"dtc_control_roc_read") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_read, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    ss << std::endl;
    Rpc_Read(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"dtc_control_roc_read_ddr") == 0) {
    ss << std::endl;
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_read_ddr");
    Rpc_ReadDDR(dtc_i,roc,ss);
  }
  else if (strcmp(cmd,"dtc_control_roc_digi_rw") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_digi_rw, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    ss << std::endl;
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_digi_rw");

    trkdaq::ControlRoc_DigiRW_Input_t  par;
    trkdaq::ControlRoc_DigiRW_Output_t pout;
    // parameters should be taken from ODB - where from?

    par.rw           = o["rw"     ];    //
    par.hvcal        = o["hvcal"  ];    //
    par.address      = o["address"];    //
    par.data[0]      = o["data"   ][0]; //
    par.data[1]      = o["data"   ][1]; //
    int  print_level = o["print_level"];
      
    printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
    try {
      dtc_i->ControlRoc_DigiRW(&par,&pout,roc,print_level,ss);
    }
    catch(...) {
      ss << "ERROR : coudn't execute ControlRoc_DigiRW ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_read_register") == 0) {
    int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC/read_register");
    
    try {
      uint32_t reg = o["Register"];
      uint32_t val;
      dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
      o["Value"] = val;
      ss << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { ss << " ERROR : dtc_read_register ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_write_register") == 0) {
    try {
      int      timeout_ms(150);
      midas::odb o("/Mu2e/Commands/Tracker/DTC/write_register");
      uint32_t reg = o["Register"];
      uint32_t val = o["Value"];
      ss << " -- write_dtc_register::0x" << std::hex << reg << " val:0x" << val << std::dec;
      dtc_i->fDtc->GetDevice()->write_register(reg,timeout_ms,val);
    }
    catch (...) { ss << "ERROR : DTC write register failed. BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_soft_reset") == 0) {
    try         { dtc_i->Dtc()->SoftReset(); ss << "soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_hard_reset") == 0) {
    try         { dtc_i->Dtc()->HardReset(); ss << "hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_init_readout") == 0) {
    try         {
      midas::odb o("/Mu2e/Commands/Tracker/DTC/init_readout");
      uint32_t emulate_cfo      = o["emulate_cfo"     ];
      uint32_t roc_readout_mode = o["roc_readout_mode"];
      dtc_i->InitReadout(emulate_cfo,roc_readout_mode);
      ss << "DTC:" << pcie_addr << " emulate_cfo:" << emulate_cfo
         << " roc_readout_mode:" << roc_readout_mode << " init readout OK";
    }
    catch (...) { ss << "ERROR : coudn't init readout DTC:" << pcie_addr; }
  }
  else if (strcmp(cmd,"dtc_print_status") == 0) {
    ss << std::endl;
    try         { dtc_i->PrintStatus(ss); }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_update_registers") == 0) {
    try         {
      ReadNonHistDtcRegisters(dtc_i);
    }
    catch (...) { ss << "ERROR : coudn't update DTC non-hist registers... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_print_roc_status") == 0) {
    ss << std::endl;
    try         { dtc_i->PrintRocStatus(1,-1,ss); }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_control_roc_find_alignment") == 0) {
    if (roc == -1)  ss << std::endl;
    try         { dtc_i->FindAlignments(1,roc,ss); }
    catch (...) { ss << " -- ERROR : coudn't execute FindAlignments for link:" << roc << " ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"get_key") == 0) {
//-----------------------------------------------------------------------------
// get key data
//-----------------------------------------------------------------------------
    ss << std::endl;
    try         {
      std::vector<uint16_t> data;
      dtc_i->ControlRoc_GetKey(data,roc,2,ss);
    }
    catch (...) { ss << "ERROR : coudn't execute ControlRoc_GetKey ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"get_roc_design_info") == 0) {
//-----------------------------------------------------------------------------
// get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    ss << std::endl;
    int rmin(roc), rmax(roc+1);
    if (roc == -1) {
      rmin = 0;
      rmax = 6;
    }
      
    TLOG(TLVL_INFO) << "-------------- rmin, rmax:" << rmin << " " << rmax;

    for (int i=rmin; i<rmax; ++i) {
      ss << "----- link:" << i ;
      if (dtc_i->LinkEnabled(i) == 0) {
        ss << " disabled, continue" << std::endl;
      }
      // ROC ID
      try         {
        std::string roc_id = dtc_i->GetRocID(i);
        ss << std::endl;
        ss << "roc_id         :" << roc_id << std::endl;
      }
      catch (...) {
        ss << "ERROR : coudn't execute GetRocID ... BAIL OUT" << std::endl;
      }
      // design info 
      try         {
        std::string design_info = dtc_i->GetRocDesignInfo(i);
        ss << "roc_design_info:" << design_info << std::endl;
      }
      catch (...) {
        ss << "ERROR : coudn't execute GetRocDesignInfo ... BAIL OUT" << std::endl;
      }
      // git commit
      try {
        std::string git_commit = dtc_i->GetRocFwGitCommit(i);
        ss << "git_commit     :" << "'"  << git_commit << "'" << std::endl;
      }
      catch (...) {
        ss << "ERROR : coudn't execute GetRocFwGitCommit ... BAIL OUT" << std::endl;
      }
    }
  }
  else if (strcmp(cmd,"measure_thresholds") == 0) {
//-----------------------------------------------------------------------------
// measure thresholds
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at measure_thresholds";
    ss << std::endl;
    ThreadContext_t cxt(pcie_addr,roc,2);
    MeasureThresholds(cxt,ss);
  }
  else if (strcmp(cmd,"load_thresholds") == 0) {
//-----------------------------------------------------------------------------
// load thresholds
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at load_thresholds";
    if (roc == -1) ss << std::endl;
    ThreadContext_t cxt(pcie_addr,roc);
    LoadThresholds(cxt,ss);             // this is fast, synchronous
  }
  else if (strcmp(cmd,"set_thresholds") == 0) {
//-----------------------------------------------------------------------------
// set thresholds: use thread as the RPC times out
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at set_thresholds";
    // ss << std::endl;
    fSetThrContext.fPcieAddr = pcie_addr;
    fSetThrContext.fLink     = roc;

    // SetThresholds(fSetThrContext,*this,ss);
    std::thread set_thr_thread(SetThresholds,
                               std::ref(fSetThrContext),
                               std::ref(*this),
                               std::ref(ss)
                               );
    set_thr_thread.detach();
    // add sleep to be able to print the output
    ss_sleep(5000);
  }
  else if (strcmp(cmd,"read_roc_register") == 0) {
//-----------------------------------------------------------------------------
// ROC registers are 16-bit
//-----------------------------------------------------------------------------
    try         {
      int timeout_ms(150);

      midas::odb o("/Mu2e/Commands/Tracker/DTC/read_roc_register");
      uint16_t reg = o["Register"];
      uint16_t val = dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(roc),reg,timeout_ms);
      o["Value"]   = val;
      ss << " -- read_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { ss << " -- ERROR : coudn't read ROC register ... BAIL OUT"; }
  }
  else if (strcmp(cmd,"dtc_control_roc_pulser_on") == 0) {
//-----------------------------------------------------------------------------
// turn pulser ON - lik is passed explicitly
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_on";
    std::string parameter_path("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on");

    int first_channel_mask, duty_cycle, pulser_delay, print_level;
    try {
      midas::odb o(parameter_path);

      first_channel_mask = o["first_channel_mask"];    //
      duty_cycle         = o["duty_cycle"        ];    //
      pulser_delay       = o["pulser_delay"      ];    //
      print_level        = o["print_level"       ];
    }
    catch(...) {
      ss << " -- ERROR : coudn't read parameters from " << parameter_path << " ... BAIL OUT" << std::endl;
    }

    TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOn, roc:" << roc;
    try {
      dtc_i->ControlRoc_PulserOn(roc,first_channel_mask,duty_cycle,pulser_delay,print_level,ss);
    }
    catch(...) {
      ss << " -- ERROR : coudn\'t execute ControlRoc_PulserOn ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_control_roc_pulser_off") == 0) {
//-----------------------------------------------------------------------------
// turn pulser OFF
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_off";
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_pulser_off");

    int print_level        = o["print_level"];

    TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOff, roc:" << roc;
    try {
      dtc_i->ControlRoc_PulserOff(roc,print_level,ss);
    }
    catch(...) {
      ss << "ERROR : coudn't execute ControlRoc_PulserOFF ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_control_roc_rates") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_rates";
    
     Rpc_Rates(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"set_caldac") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
    
     Rpc_SetCalDac(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"dump_settings") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    ss << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dump_settings";
    
    Rpc_DumpSettings(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"read_ilp") == 0) {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    ss << std::endl;
    try         {
      std::vector<uint16_t>   data;
      dtc_i->ControlRoc_ReadIlp(data,roc,2,ss);
    }
    catch (...) { ss << "ERROR : coudn't read ILP ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"read_spi") == 0) {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    ss << std::endl;
    try         {
      if (roc != -1) {
        std::vector<uint16_t>   data;
        dtc_i->ControlRoc_ReadSpi(data,roc,2,ss);
      }
      else {
        // need formatted printout for all ROCs
        trkdaq::TrkSpiData_t spi[6];
        for (int i=0; i<6; i++) {
          if (dtc_i->LinkEnabled(i)) {
            dtc_i->ControlRoc_ReadSpi_1(&spi[i],i,0,ss);
          }
        }
        // now - printing
        dtc_i->PrintSpiAll(spi,ss);
      }
    }
    catch (...) { ss << "ERROR : coudn't read SPI ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"write_roc_register") == 0) {
    try         {
      int timeout_ms(150);

      midas::odb o("/Mu2e/Commands/Tracker/DTC/write_roc_register");
      uint16_t reg = o["Register"];
      uint16_t val = o["Value"];
      dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(roc),reg,val,false,timeout_ms);
      ss << " -- write_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { ss << "ERROR : coudn't write ROC register ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"print_roc_status") == 0) {
    ss << std::endl;
    try         {
      dtc_i->PrintRocStatus(1,1<<4*roc,ss);
      ss << " -- print_roc_status : emoe";
    }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"reset_roc") == 0) {
    try         {
      int mask = 1<<4*roc;
      dtc_i->ResetLinks(mask);
      ss << " -- reset_roc OK";
    }
    catch (...) { ss << "ERROR : coudn't reset ROC:" << roc << " ... BAIL OUT" << std::endl; }
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }

  response += ss.str();

  TLOG(TLVL_DEBUG) << "-- END: response:" << response;

  ss.str("");

  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TTrkEquipmentNode::HandleBinaryRpc(const char* cmd, const char* args, std::vector<char>& response) {
  fMfe->Msg(MINFO, "HandleBinaryRpc", "RPC cmd [%s], args [%s]", cmd, args);

  // RPC handler
      
  response.resize(8*64);

  uint64_t* p64 = (uint64_t*)response.data();

  for (size_t i=0; i<response.size()/sizeof(p64[0]); i++) {
    *p64++ = (1<<i);
  }
      
  return TMFeOk();
}

