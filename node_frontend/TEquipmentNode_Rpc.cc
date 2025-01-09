/////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode_Rpc"
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleRpc(const char* cmd, const char* args, std::string& response) {
  fMfe->Msg(MINFO, "HandleRpc", "RPC cmd [%s], args [%s]", cmd, args);

  std::stringstream ss;

  TLOG(TLVL_DEBUG) << "RPC cmd:" << cmd << " args:" << args;
  
  time_t now = time(NULL);
  char tmp[256];
  sprintf(tmp, "{ \"hey, emoe, current_time\":[ %d, \"%s\"] }", (int)now, ctime(&now));
  
  if (strcmp(cmd,"dtc_control_roc_read") == 0) {

    midas::odb o = {};
    o.connect("/Mu2e/Commands/Tracker/DTC/control_ROC_read");
    for (int i=0; i<2; i++) {
      trkdaq::DtcInterface* dtc_i = fDtc_i[i];
      if (dtc_i) {
        trkdaq::ControlRoc_Read_Input_t par;
        // parameters should be taken from ODB - where from? 
  
        par.adc_mode        = o["adc_mode"     ];   // -a
        par.tdc_mode        = o["tdc_mode"     ];   // -t 
        par.num_lookback    = o["num_lookback" ];   // -l 
        par.num_samples     = o["num_samples"  ];   // -s
        par.num_triggers[0] = o["num_triggers"][0]; // -T 10
        par.num_triggers[1] = o["num_triggers"][1]; //
        
        par.ch_mask[0]      = o["ch_mask"][0];
        par.ch_mask[1]      = o["ch_mask"][1];
        par.ch_mask[2]      = o["ch_mask"][2];
        par.ch_mask[3]      = o["ch_mask"][3];
        par.ch_mask[4]      = o["ch_mask"][4];
        par.ch_mask[5]      = o["ch_mask"][5];
        
        par.enable_pulser   = o["enable_pulser"];   // -p 1
        par.marker_clock    = o["marker_clock" ];   // -m 3
        par.mode            = o["mode"         ];   // 
        par.clock           = o["clock"        ];   // 

        printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
        bool update_mask(false);
        int  print_level(0);
        dtc_i->ControlRoc_Read(&par,-1,update_mask,print_level);
      }
    }
    ss << "Hopefully, success of dtc_control_roc_read";
  }
  else if (strcmp(cmd,"dtc_read_register") == 0) {
    int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC");
    int pcie_addr = o["PcieAddress"];
    trkdaq::DtcInterface* dtc_i = fDtc_i[pcie_addr];
    
    uint32_t reg = o["read_register/Register"];
    uint32_t val;
    try {
      dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
      o["read_register/Value"] = val;
      ss << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_soft_reset") == 0) {
    //    int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC");
    int pcie_addr = o["PcieAddress"];
    trkdaq::DtcInterface* dtc_i = fDtc_i[pcie_addr];
    
    try         { dtc_i->Dtc()->SoftReset(); ss << "soft reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_hard_reset") == 0) {
    //    int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC");
    int pcie_addr = o["PcieAddress"];
    trkdaq::DtcInterface* dtc_i = fDtc_i[pcie_addr];
    
    try         { dtc_i->Dtc()->HardReset(); ss << "hard reset OK" << std::endl; }
    catch (...) { ss << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_init_readout") == 0) {

    for (int i=0; i<2; i++) {
      try         {
        trkdaq::DtcInterface* dtc_i = fDtc_i[i];
        if (dtc_i) {
          dtc_i->InitReadout();
          ss << "DTC:" << i << " init readout OK";
        }
      }
      catch (...) {
        ss << "ERROR : coudn't init readout DTC:" << i;
      }
    }
  }
  else if (strcmp(cmd,"dtc_print_status") == 0) {
    // int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC");
    int pcie_addr = o["PcieAddress"];
    trkdaq::DtcInterface* dtc_i = fDtc_i[pcie_addr];
    
    try         { dtc_i->PrintStatus(ss); }
    catch (...) { ss << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
    
  }
  else {
    ss << "ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }

  response = ss.str();

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

