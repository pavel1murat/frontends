//
#include <fstream>
#include "utils/utils.hh"
#include "node_frontend/TMu2eEqBase.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TMu2eEqBase"

//-----------------------------------------------------------------------------
TMu2eEqBase::TMu2eEqBase() {
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
void TMu2eEqBase::ResetOutput() {
  TLOG(TLVL_DEBUG) << "--- START"; 

  std::ofstream output_file;
  output_file.open(_logfile,std::ofstream::trunc);
  output_file.close();

  //  ss_sleep(100);

  // midas::odb o_cmd("/Mu2e/Commands/Tracker");
  // o_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "--- END"; 
}

//-----------------------------------------------------------------------------
void TMu2eEqBase::WriteOutput(const std::string& Output) {
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
}
