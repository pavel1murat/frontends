//
#include <fstream>
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "node_frontend/TMu2eEqBase.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TMu2eEqBase"

//-----------------------------------------------------------------------------
TMu2eEqBase::TMu2eEqBase() {
  
  _odb_i                      = OdbInterface::Instance();
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  std::string private_subnet  = _odb_i->GetPrivateSubnet(_h_active_run_conf);
  std::string public_subnet   = _odb_i->GetPublicSubnet (_h_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _host_label      = get_short_host_name(public_subnet.data());
  _full_host_name  = get_full_host_name (private_subnet.data());
  _h_daq_host_conf = _odb_i->GetHostConfHandle(_host_label);
  _monitoringLevel = 0;
}

//-----------------------------------------------------------------------------
TMu2eEqBase::~TMu2eEqBase() {
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::BeginRun(HNDLE H_RunConf) {
  return 0;
}

//-----------------------------------------------------------------------------
TMFeResult TMu2eEqBase::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::InitVarNames() {
  return 0;
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::ReadMetrics() {
  return 0;
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::ResetOutput() {
  TLOG(TLVL_DEBUG) << "--- START"; 

  std::ofstream output_file;
  output_file.open(_logfile,std::ofstream::trunc);
  output_file.close();

  //  ss_sleep(100);

  // midas::odb o_cmd("/Mu2e/Commands/Tracker");
  // o_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::WriteOutput(const std::string& Output) {
  TLOG(TLVL_DEBUG) << "--- START"; 

  std::vector<std::string> vs = splitString(Output,'\n');
  
  std::ofstream output_file;
  output_file.open(_logfile.data(),std::ios::app);

  int ns = vs.size();
  for (int i=ns-1; i>=0; i--) {
    output_file << vs[i] << std::endl;
  }

  output_file.close();
  
  TLOG(TLVL_DEBUG) << "--- END"; 
  return 0;
}
