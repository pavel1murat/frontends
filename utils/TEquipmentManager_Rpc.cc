/////////////////////////////////////////////////////////////////////////////
#include <format>

#include "utils/TEquipmentManager.hh"
#include "utils/TMu2eEqBase.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

using json = nlohmann::json;

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentManager"

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
  int            pcie_addr(-1), link(-1);
  std::string    eq_type;
  TMu2eEqBase*   eq (nullptr);
  
  json j1;
  try {
    TLOG(TLVL_DEBUG) << "before parsing json args:" << args;
    j1 = nlohmann::json::parse(args);
  }
  catch(const std::exception& e) {
    std::string msg("couldn't parse json:");
    TLOG(TLVL_ERROR) << msg << " err:" << e.what();
    return TMFeErrorMessage(msg+args);
  }
    
  TLOG(TLVL_DEBUG) << "PM: done parsing";
  
  // equipment type is always defined, the rest parameters depend on it
  eq_type        = j1.at("eq_type");

  std::string eq_name = eq_type;
  
  if (eq_type == "dtc") {
    pcie_addr = j1.at("pcie");
    eq_name   = std::format("dtc{}",pcie_addr);
    link      = j1.at("roc" );
  }
//-----------------------------------------------------------------------------
// equipment namse are always capitalized
//-----------------------------------------------------------------------------
  std::transform(eq_name.begin(),eq_name.end(),eq_name.begin(),::toupper);
  eq          = FindEquipmentItem(eq_name);
  
  if (eq != nullptr) {
    TLOG(TLVL_DEBUG) << std::format("eq_name:{} eq:{} ",eq_name,(void*) eq);
  }
  else {
    TLOG(TLVL_ERROR) << "undefined eq_type:" << eq_type;
    throw std::runtime_error("undefined eq_type:"+eq_type);
  }
//-----------------------------------------------------------------------------
// start forming response
//-----------------------------------------------------------------------------
  response  = "";
//-----------------------------------------------------------------------------
// a single place to make sure that the pointer to the EqDTC is OK
//-----------------------------------------------------------------------------
  if (eq == nullptr) {
    ss << "EQ:" << eq_type << " is disabled";
    response += ss.str();
    TLOG(TLVL_ERROR) << response;
    return TMFeErrorMessage(response);
  }
//-----------------------------------------------------------------------------
// eq_name: 'DTC0', 'DTC1', 'Artdaq', 'Disk', etc
// for eq_name = DTC, there is link defined - set it in the command parameters
// in ODB
//-----------------------------------------------------------------------------
  std::string cmd_path = odb_i->GetCmdConfigPath(_host_label,eq->Title());
  HNDLE       h_cmd    = odb_i->GetHandle(0,cmd_path);
  TLOG(TLVL_DEBUG) << std::format("cmd_path:{} h_cmd:{} eq->Name():{} eq->Title():{}",
                                  cmd_path,h_cmd,eq->Name(),eq->Title());
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
    odb_i->SetInteger(h_cmd,"link",link);
  }
  // and , finally, trigger the execution
  odb_i->SetInteger(h_cmd,"Run"          ,1);
  // after that, return to the caller, with the response "passed"

  response += " passed for execution";
  
  TLOG(TLVL_DEBUG) << "-- END: response:" << response;
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

