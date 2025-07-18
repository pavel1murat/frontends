/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentManager.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

using json = nlohmann::json;

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentManager"

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
// this function needs to define which type of equipment is being talked to and forward
//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleRpc(const char* cmd, const char* args, std::string& response) {
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
  int pcie_addr(-1); // , roc(-1);
  std::string eq_type;

  TMu2eEqBase* eq (nullptr);
  
  try {
    json j1 = json::parse(args);
    TLOG(TLVL_DEBUG) << "json string:" << j1;
    
    // equipment type is always defined, the rest parameters depend on it
    eq_type        = j1.at("eq_type" );
    if (eq_type == "dtc") {
      pcie_addr = j1.at("pcie");
      eq        = _eq_dtc[pcie_addr];
      TLOG(TLVL_DEBUG) << "pcie_addr:" << pcie_addr << " eq:" << eq << " _eq_dtc[0]:0x" << std::hex << _eq_dtc[0];
    }
    else if (eq_type == "artdaq") {
                                        // perhaps some additional parameters to parse
      eq = _eq_artdaq;
    }
    else if (eq_type == "disk") {
                                        // perhaps some additional parameters to parse
      eq = _eq_disk;
    }
    else {
      TLOG(TLVL_ERROR) << "undefined eq_type:" << eq_type;
      throw std::runtime_error("undefined eq_type:"+eq_type);
    }      
  }
  catch(...) {
    std::string msg("couldn't parse json:");
    TLOG(TLVL_ERROR) << msg << args;
    return TMFeErrorMessage(msg+args);
  }
//-----------------------------------------------------------------------------
// start forming response
//-----------------------------------------------------------------------------
  response  = "cmd:";
  response += cmd;
  response += " args:";
  response += args;
//-----------------------------------------------------------------------------
// a single place to make sure that the pointer to the EqDTC is OK
//-----------------------------------------------------------------------------
  if (eq == nullptr) {
    ss << " EQ:" << eq_type << " is not enabled";
    response += ss.str();
    TLOG(TLVL_ERROR) << response;
    return TMFeErrorMessage(response);
  }
//-----------------------------------------------------------------------------
// eq_name: 'DTC0', 'DTC1', 'Artdaq', 'Disk', etc
//-----------------------------------------------------------------------------
  std::string cmd_path = odb_i->GetCmdConfigPath(_host_label,eq->Name());
  HNDLE       h_cmd    = odb_i->GetHandle(0,cmd_path);
  TLOG(TLVL_DEBUG) << "cmd_path:" << cmd_path << " h_cmd:" << h_cmd << "eq->Name():" << eq->Name();
//-----------------------------------------------------------------------------
// is the assumption that the parameter path is always local OK ?
//-----------------------------------------------------------------------------
  std::string cmd_parameter_path = std::format("{}/{}",cmd_path,cmd);

  TLOG(TLVL_DEBUG) << "cmd_parameter_path:" << cmd_parameter_path;

  std::string s_cmd(cmd);
  
  odb_i->SetString (h_cmd,"Name"         ,s_cmd);
  odb_i->SetString (h_cmd,"ParameterPath",cmd_parameter_path);
  odb_i->SetInteger(h_cmd,"Finished"     ,0);
  odb_i->SetInteger(h_cmd,"ReturnCode"   ,0);
  // and , finally, trigger the execution
  odb_i->SetInteger(h_cmd,"Run"          ,1);
  // after that, return to the caller, with the response "passed"

  response += " passed for execution";
  
  return TMFeOk();


  // else if (strcmp(cmd,"dtc_control_roc_find_alignment") == 0) {
  // }
//   else if (strcmp(cmd,"load_thresholds") == 0) {
// //-----------------------------------------------------------------------------
// // load thresholds
// //-----------------------------------------------------------------------------
//     TLOG(TLVL_DEBUG) << "arrived at load_thresholds";
//     if (roc == -1) ss << std::endl;
//     ThreadContext_t cxt(pcie_addr,roc);
//     LoadThresholds(cxt,ss);             // this is fast, synchronous
//   }
//   else if (strcmp(cmd,"set_thresholds") == 0) {
// //-----------------------------------------------------------------------------
// // set thresholds: use thread as the RPC times out
// //-----------------------------------------------------------------------------
//     TLOG(TLVL_DEBUG) << "arrived at set_thresholds";
//     // ss << std::endl;
//     fSetThrContext.fPcieAddr = pcie_addr;
//     fSetThrContext.fLink     = roc;

//     // SetThresholds(fSetThrContext,*this,ss);
//     std::thread set_thr_thread(SetThresholds,
//                                std::ref(fSetThrContext),
//                                std::ref(*this),
//                                std::ref(ss)
//                                );
//     set_thr_thread.detach();
//     // add sleep to be able to print the output
//     ss_sleep(5000);
//   }
//   else if (strcmp(cmd,"read_roc_register") == 0) {
// //-----------------------------------------------------------------------------
// // ROC registers are 16-bit
// //-----------------------------------------------------------------------------
//     try         {
//       int timeout_ms(150);

//       midas::odb o("/Mu2e/Commands/Tracker/DTC/read_roc_register");
//       uint16_t reg = o["Register"];
//       uint16_t val = dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(roc),reg,timeout_ms);
//       o["Value"]   = val;
//       ss << " -- read_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
//     }
//     catch (...) { ss << " -- ERROR : coudn't read ROC register ... BAIL OUT"; }
//   }
//   else if (strcmp(cmd,"dtc_control_roc_pulser_on") == 0) {
// //-----------------------------------------------------------------------------
// // turn pulser ON - lik is passed explicitly
// //-----------------------------------------------------------------------------
//     TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_on";
//     std::string parameter_path("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on");

//     int first_channel_mask, duty_cycle, pulser_delay, print_level;
//     try {
//       midas::odb o(parameter_path);

//       first_channel_mask = o["first_channel_mask"];    //
//       duty_cycle         = o["duty_cycle"        ];    //
//       pulser_delay       = o["pulser_delay"      ];    //
//       print_level        = o["print_level"       ];
//     }
//     catch(...) {
//       ss << " -- ERROR : coudn't read parameters from " << parameter_path << " ... BAIL OUT" << std::endl;
//     }

//     TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOn, roc:" << roc;
//     try {
//       dtc_i->ControlRoc_PulserOn(roc,first_channel_mask,duty_cycle,pulser_delay,print_level,ss);
//     }
//     catch(...) {
//       ss << " -- ERROR : coudn\'t execute ControlRoc_PulserOn ... BAIL OUT" << std::endl;
//     }
//   }
//   else if (strcmp(cmd,"dtc_control_roc_pulser_off") == 0) {
// //-----------------------------------------------------------------------------
// // turn pulser OFF
// //-----------------------------------------------------------------------------
//     TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_off";
//     midas::odb o("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_off");

//     int print_level        = o["print_level"];

//     TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOff, roc:" << roc;
//     try {
//       dtc_i->ControlRoc_PulserOff(roc,print_level,ss);
//     }
//     catch(...) {
//       ss << "ERROR : coudn't execute ControlRoc_PulserOFF ... BAIL OUT" << std::endl;
//     }
//   }
  // else if (strcmp(cmd,"write_roc_register") == 0) {
  //   try         {
  //     int timeout_ms(150);

  //     midas::odb o("/Mu2e/Commands/Tracker/DTC/write_roc_register");
  //     uint16_t reg = o["Register"];
  //     uint16_t val = o["Value"];
  //     dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(roc),reg,val,false,timeout_ms);
  //     ss << " -- write_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  //   }
  //   catch (...) { ss << "ERROR : coudn't write ROC register ... BAIL OUT" << std::endl; }
  // }
  // else if (strcmp(cmd,"print_roc_status") == 0) {
  //   ss << std::endl;
  //   try         {
  //     dtc_i->PrintRocStatus(1,1<<4*roc,ss);
  //     ss << " -- print_roc_status : emoe";
  //   }
  //   catch (...) { ss << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  // }

  response += ss.str();

  TLOG(TLVL_DEBUG) << "-- END: response:" << response;

  ss.str("");

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentManager::HandleBinaryRpc(const char* cmd, const char* args, std::vector<char>& response) {
  fMfe->Msg(MINFO, "HandleBinaryRpc", "RPC cmd [%s], args [%s]", cmd, args);

  // RPC handler
   
  response.resize(8*64);

  uint64_t* p64 = (uint64_t*)response.data();

  for (size_t i=0; i<response.size()/sizeof(p64[0]); i++) {
    *p64++ = (1<<i);
  }
   
  return TMFeOk();
}

