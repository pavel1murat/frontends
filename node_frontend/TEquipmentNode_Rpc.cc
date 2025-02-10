/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

using json = nlohmann::json;

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode_Rpc"

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
TMFeResult TEquipmentNode::HandleRpc(const char* cmd, const char* args, std::string& response) {
  fMfe->Msg(MINFO, "HandleRpc", "RPC cmd [%s], args [%s]", cmd, args);

  std::stringstream ss;

  TLOG(TLVL_DEBUG) << "RPC cmd:" << cmd << " args:" << args;
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

  // time_t now = time(NULL);
  // char tmp[256];
  // sprintf(tmp, "{ \"hey, current_time\":[ %d, \"%s\"] }", (int)now, ctime(&now));
  
  trkdaq::DtcInterface* dtc_i = dynamic_cast<trkdaq::DtcInterface*>(fDtc_i[pcie_addr]);
  
  if (dtc_i == nullptr) {
    ss << "DTC" << pcie_addr << " is not enabled";
    response = ss.str();
    TLOG(TLVL_ERROR) << response << args;
    return TMFeErrorMessage(response+args);
  }
  
  if (strcmp(cmd,"dtc_control_roc_read") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_read, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_read");

    trkdaq::ControlRoc_Read_Input_t par;
    // parameters should be taken from ODB - where from?

    par.version         = o["version"      ];
    par.adc_mode        = o["adc_mode"     ];   // -a
    par.tdc_mode        = o["tdc_mode"     ];   // -t 
    par.num_lookback    = o["num_lookback" ];   // -l 

    if (par.version == 1) {
      par.v1.num_samples     = o["num_samples"  ];   // -s
      par.v1.num_triggers[0] = o["num_triggers"][0]; // -T 10
      par.v1.num_triggers[1] = o["num_triggers"][1]; //
        
      par.v1.ch_mask[0]      = o["ch_mask"][0];
      par.v1.ch_mask[1]      = o["ch_mask"][1];
      par.v1.ch_mask[2]      = o["ch_mask"][2];
      par.v1.ch_mask[3]      = o["ch_mask"][3];
      par.v1.ch_mask[4]      = o["ch_mask"][4];
      par.v1.ch_mask[5]      = o["ch_mask"][5];
        
      par.v1.enable_pulser   = o["enable_pulser"];   // -p 1
      par.v1.marker_clock    = o["marker_clock" ];   // -m 3
      par.v1.mode            = o["mode"         ];   // 
      par.v1.clock           = o["clock"        ];   //
    }
    else if (par.version == 2) {
      par.v2.num_samples     = o["num_samples"  ];   // -s
      par.v2.num_triggers[0] = o["num_triggers"][0]; // -T 10
      par.v2.num_triggers[1] = o["num_triggers"][1]; //
        
      par.v2.ch_mask[0]      = o["ch_mask"][0];
      par.v2.ch_mask[1]      = o["ch_mask"][1];
      par.v2.ch_mask[2]      = o["ch_mask"][2];
      par.v2.ch_mask[3]      = o["ch_mask"][3];
      par.v2.ch_mask[4]      = o["ch_mask"][4];
      par.v2.ch_mask[5]      = o["ch_mask"][5];
        
      par.v2.enable_pulser   = o["enable_pulser"];   // -p 1
      par.v2.marker_clock    = o["marker_clock" ];   // -m 3
      par.v2.mode            = o["mode"         ];   // 
      par.v2.clock           = o["clock"        ];   //
    }
      
    printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
    int  print_level(3);
    try {
      dtc_i->ControlRoc_Read(&par,-1,print_level,ss);
    }
    catch(...) {
      ss << "ERROR : coudn't execute ControlRoc_Read ... BAIL OUT" << std::endl;
    }
  }
  if (strcmp(cmd,"dtc_control_roc_digi_rw") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_read, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_digi_rw");

    trkdaq::ControlRoc_DigiRW_Input_t  par;
    trkdaq::ControlRoc_DigiRW_Output_t pout;
    // parameters should be taken from ODB - where from?

    par.rw         = o["rw"     ];    //
    par.hvcal      = o["hvcal"  ];    //
    par.address    = o["address"];    //
    par.data[0]    = o["data"   ][0]; //
    par.data[1]    = o["data"   ][1]; //
      
    printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
    int  print_level(3);
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
    catch (...) { ss << "ERROR : dtc_read_register ... BAIL OUT" << std::endl; }
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
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
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
      dtc_i->InitReadout();
      ss << "DTC:" << pcie_addr << " init readout OK";
    }
    catch (...) { ss << "ERROR : coudn't init readout DTC:" << pcie_addr; }
  }
  else if (strcmp(cmd,"dtc_print_status") == 0) {
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
    try         { dtc_i->PrintRocStatus(1,-1,ss); }
    catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
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
    catch (...) { ss << "ERROR : coudn't read ROC register ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"read_spi") == 0) {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    try         {
      std::vector<uint16_t>   spi_data;
      dtc_i->ControlRoc_ReadSpi(spi_data,roc,2,ss);
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
    ss << "ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }

  response  = "args:";
  response += args;
  response += ";\n";
  response += ss.str();

  TLOG(TLVL_DEBUG) << "response:" << response;

  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleBinaryRpc(const char* cmd, const char* args, std::vector<char>& response) {
  fMfe->Msg(MINFO, "HandleBinaryRpc", "RPC cmd [%s], args [%s]", cmd, args);

  // RPC handler
      
  response.resize(8*64);

  uint64_t* p64 = (uint64_t*)response.data();

  for (size_t i=0; i<response.size()/sizeof(p64[0]); i++) {
    *p64++ = (1<<i);
  }
      
  return TMFeOk();
}

