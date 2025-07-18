/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
// takes parameters from ODB
// a DTC can execute one command at a time
// this command is fast,
// equipment knows the node name and has an interface to ODB
//-----------------------------------------------------------------------------
int TEqTrkDtc::ConfigureJA(std::ostream& Stream, const char* ConfName) {

  TLOG(TLVL_DEBUG) << "--- START";

  std::string path     = std::format("/Mu2e/Commands/DAQ/Nodes/{}/DTC{}",_host_label,_dtc_i->PcieAddr());

  midas::odb o_cmd(cmd_path.data());

  std::string parameter_path = o_cmd["ParameterPath"];
  o_cmd["Finished"] = 0;
  
  std::string cmd_path = path+"/configure_ja";
              
  int print_level      = o_cmd["print_level"];

  int rc = _dtc_i->ConfigureJA();

  o_cmd["ReturnCode"] = rc;
  o_cmd["Run"       ] =  0;
  o_cmd["Finished"  ] =  1;
  
  TLOG(TLVL_DEBUG) << "--- END : rc:" << rc;
  return 0;
}
