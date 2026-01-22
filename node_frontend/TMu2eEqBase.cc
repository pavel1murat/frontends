//
#include <fstream>
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "node_frontend/TMu2eEqBase.hh"
#include <format>

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TMu2eEqBase"

//-----------------------------------------------------------------------------
TMu2eEqBase::TMu2eEqBase(const char* EqName) {
  TLOG(TLVL_DEBUG) << std::format("-- START EqName:{}",EqName); 

  _name                       = EqName;
  _odb_i                      = OdbInterface::Instance();
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  std::string private_subnet  = _odb_i->GetPrivateSubnet(_h_active_run_conf);
  std::string public_subnet   = _odb_i->GetPublicSubnet (_h_active_run_conf);
  TLOG(TLVL_DEBUG) << std::format(" private_subnet:{} public_subnet:{}",private_subnet,public_subnet); 
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _host_label      = get_short_host_name(public_subnet.data());
  _full_host_name  = get_full_host_name (private_subnet.data());
  _h_daq_host_conf = _odb_i->GetHostConfHandle(_host_label);
  _monitoringLevel = 0;
  TLOG(TLVL_DEBUG) << std::format("-- END _host_label:{} _full_host_name:{}",_host_label,_full_host_name); 
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
int TMu2eEqBase::HandlePeriodic() {
  return 0;
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::ResetOutput() {
  TLOG(TLVL_DEBUG) << "--- START _logfile:" << _logfile; 

  std::ofstream output_file;
  output_file.open(_logfile,std::ofstream::trunc);
  if (not output_file.is_open()) {
    TLOG(TLVL_ERROR) << std::format("failed to open _logfile:{} in ofstream::trunc mode",_logfile); 
  }
  else {
    output_file.close();
  }

  //  ss_sleep(100);

  // midas::odb o_cmd("/Mu2e/Commands/Tracker");
  // o_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
// make sure that a command can redirect its output
int TMu2eEqBase::WriteOutput(const std::string& Output, const std::string& Logfile) {

  TLOG(TLVL_DEBUG) << std::format("-- START: _logfile:{} Logfile:{} Output size:{}",_logfile,Logfile,Output.length()); 

  std::vector<std::string> vs = splitString(Output,'\n');

  std::mutex mtx; // For thread-safe output

  std::string fn = _logfile;
  if (Logfile != "") {
    std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
    fn = std::format("{}/{}",data_dir,Logfile);
  }

  TLOG(TLVL_DEBUG) << std::format("using fn:{}",fn);
//-----------------------------------------------------------------------------
// make sure writing to disk is thread-safe
//-----------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> lock(mtx);

    std::ofstream output_file;
    output_file.open(fn.data(),std::ios::app);
    if (not output_file.is_open()) {
      TLOG(TLVL_ERROR) << std::format("failed to open log file:{} in ios::app mode",fn); 
    }
    else {
      int ns = vs.size();
      for (int i=ns-1; i>=0; i--) {
        output_file << vs[i] << std::endl;
        TLOG(TLVL_DEBUG+1) << vs[i];
      }
      output_file.close();
    }
  }
  
  TLOG(TLVL_DEBUG) << "-- END"; 
  return 0;
}
