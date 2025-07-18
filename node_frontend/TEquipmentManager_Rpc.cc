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
  int pcie_addr(-1), link(-1);
  std::string eq_type;

  TMu2eEqBase* eq (nullptr);
  
  try {
    json j1 = json::parse(args);
    TLOG(TLVL_DEBUG) << "json string:" << j1;
    
    // equipment type is always defined, the rest parameters depend on it
    eq_type        = j1.at("eq_type" );
    if (eq_type == "dtc") {
      pcie_addr = j1.at("pcie");
      link      = j1.at("roc" );
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
// for eq_name = DTC, there is link defined - set it in the command parameters
// in ODB
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
//-----------------------------------------------------------------------------
// for DTC, in most cases need the link to be specified
//-----------------------------------------------------------------------------
  if (eq->Name().find("DTC") == 0) {
    odb_i->SetInteger(h_cmd,"Link",link);
  }
  // and , finally, trigger the execution
  odb_i->SetInteger(h_cmd,"Run"          ,1);
  // after that, return to the caller, with the response "passed"

  response += " passed for execution";
  
  return TMFeOk();

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

