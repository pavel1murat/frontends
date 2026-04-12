//
#include <fstream>
#include "midas.h"
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "utils/TMu2eEqBase.hh"
#include <format>

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TMu2eEqBase"

//-----------------------------------------------------------------------------
TMu2eEqBase::TMu2eEqBase(const char* Name, const char* Title, int Subsystem) :
  _name     (Name     ),
  _title    (Title    ),
  _subsystem(Subsystem)
{
  TLOG(TLVL_DEBUG) << std::format("-- START: _name:{} _title:{}",_name,_title); 

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
  _h_daq_host_conf = _odb_i->GetHostConfHandle(_h_active_run_conf,_host_label);
  _monitoringLevel = 0;
  TLOG(TLVL_DEBUG) << std::format("-- END: _host_label:{} _full_host_name:{}",_host_label,_full_host_name); 
}

//-----------------------------------------------------------------------------
TMu2eEqBase::~TMu2eEqBase() {
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::BeginRun(int RunNumber) {
  return 0;
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::EndRun(int RunNumber) {
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
int TMu2eEqBase::CheckAlarms() {
  return 0;
}

//-----------------------------------------------------------------------------
// the logfiles are supposed to be located in ${ODB("/Logger/Data dir")}/logs)
// returned directory name ends with the '/'
//-----------------------------------------------------------------------------
std::string TMu2eEqBase::GetFullLogfileName(const std::string& Logfile) {
  TLOG(TLVL_DEBUG+1) << std::format("-- START: Logfile:{}",Logfile);

  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
                                        // no need in the intermediate '/' here
  std::string full_fn  = std::format("{}/logs/{}",data_dir,Logfile);

  TLOG(TLVL_DEBUG+1) << std::format("-- END full_fn:{}",full_fn);
  return full_fn;
}

//-----------------------------------------------------------------------------
// the logfiles are supposed to be located in ODB("/Logger/Data dir")
//-----------------------------------------------------------------------------
int TMu2eEqBase::ResetOutput(HNDLE H_Cmd) {
  int rc(0);

  std::string logstream = _odb_i->GetString (H_Cmd,"logfile" );

  TLOG(TLVL_DEBUG) << "--- START LogStream:" << logstream; 

  SetStatus(1);                         // show as BUSY
  
  if (logstream == "") {
    rc = -1;
    TLOG(TLVL_ERROR) << std::format("logstream is not defined, rc:{}. BAIL OUT",rc);
    SetCommandFinished(H_Cmd,rc);
    return rc;
  }

  std::mutex mtx; // For thread-safe output

  {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream output_file;

    std::string fn = GetFullLogfileName(logstream+".msg");
    
    output_file.open(fn.data(),std::ofstream::trunc);
    if (not output_file.is_open()) {
      rc = -2;
      TLOG(TLVL_ERROR) << std::format("failed to open fn:{} in ofstream::trunc mode",fn); 
    }
    else {
      output_file.close();
    }
  }

  SetCommandFinished(H_Cmd,rc);
  
  TLOG(TLVL_DEBUG) << std::format("--- END, rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
// mark command as completed, set status
//-----------------------------------------------------------------------------
void TMu2eEqBase::SetCommandFinished(HNDLE H_Cmd, int Status) {
  _odb_i->SetStatus(_handle,Status);
  _odb_i->SetInteger(H_Cmd,"Finished",1);
};

//-----------------------------------------------------------------------------
void TMu2eEqBase::SetStatus(int Status) {
  _odb_i->SetStatus(_handle,Status);
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::GetStatus() {
  return _odb_i->GetStatus(_handle);
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::GetEnabled() {
  return _odb_i->GetEnabled(_handle);
}

//-----------------------------------------------------------------------------
int TMu2eEqBase::StartMessage(HNDLE h_Cmd, std::stringstream& Stream) {

  Stream << std::endl; // perhaps

  std::string cmd  = _odb_i->GetString (h_Cmd,"Name");
  
  Stream << std::format("-- cmd:{} WARNING : SHOULD BE OVERWTITTEN BY THE DERIVED CLASS",cmd);
  Stream << std::endl;
  
  return 0;
}

//-----------------------------------------------------------------------------
// set DTC in ERROR
//-----------------------------------------------------------------------------
int TMu2eEqBase::UnknownCommand(HNDLE H_Cmd) {
  int rc(-1);
  
  TLOG(TLVL_DEBUG) << "-- START";

  std::string cmd_name = _odb_i->GetString (H_Cmd,"Name"   );
  std::string logfile  = _odb_i->GetString (H_Cmd,"logfile");
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  sstr << std::format("ERROR: UNKNOWN COMMAND:{}\n",cmd_name);

  TLOG(TLVL_ERROR) << std::format("unknown command:{}",cmd_name);

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc);

  std::string msg = std::format("cmd:{} is not implemented yet",cmd_name);
  cm_msg(MERROR, __func__,msg.data());
  cm_msg_flush_buffer();
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
// make sure that a command can redirect its output
//-----------------------------------------------------------------------------
int TMu2eEqBase::WriteOutput(const std::string& Output, const std::string& LogStream, int Mode) {

  TLOG(TLVL_DEBUG) << std::format("-- START: _logfile:{} LogStream:{} Mode:{} Output size:{}",_logfile,LogStream,Mode,Output.length()); 

  std::vector<std::string> vs = splitString(Output,'\n');
  int ns = vs.size();

  std::mutex mtx;                       // For thread-safe output

  if (LogStream == "") {
    TLOG(TLVL_ERROR) << std::format("LogStream is not defined, BAIL OUT");
    return -1;
  }
//-----------------------------------------------------------------------------
// assume all logfiles are in the same directory - is it a good assumption ?
// ${experiment}/logfiles ?  mu2edaq22_dtc0.log
// make sure writing to disk is thread-safe
//-----------------------------------------------------------------------------
  {
    std::lock_guard<std::mutex> lock(mtx);

    if (Mode == 0) {
                                        // today's default: use MIDAS facility

      std::string fn = GetFullLogfileName(LogStream);
      TLOG(TLVL_DEBUG) << std::format("using fn:{}",fn);

      std::ofstream output_file;
      output_file.open(fn.data(),std::ios::app);
      if (not output_file.is_open()) {
        TLOG(TLVL_ERROR) << std::format("failed to open log file:{} in ios::app mode",fn); 
      }
      else {
        for (int i=ns-1; i>=0; i--) {
          output_file << vs[i] << std::endl;
          TLOG(TLVL_DEBUG+1) << vs[i];
        }
      }
      output_file.close();
    }
    else if (Mode == 1) {
//-----------------------------------------------------------------------------
// use node-js
// 1. write last message into a separate file : 
//-----------------------------------------------------------------------------
      std::string cmd_output_fn = GetFullLogfileName(LogStream+".msg");

      std::ofstream cmd_output;
      cmd_output.open(cmd_output_fn.data(),std::ios::out);
      if (not cmd_output.is_open()) {
        TLOG(TLVL_ERROR) << std::format("failed to open message file:{} in ios::out mode",cmd_output_fn); 
      }
      
      for (int i=0; i<ns; i++) {
        cmd_output << vs[i] << std::endl;
        TLOG(TLVL_DEBUG+1) << vs[i];
      }
      cmd_output.close();
//-----------------------------------------------------------------------------
// append message to the log file - that should be a rolling one...
//-----------------------------------------------------------------------------
      std::string   log_fn = GetFullLogfileName(LogStream+".log");
      std::ofstream log_file;
      log_file.open(log_fn,std::ios::app);
      if (not log_file.is_open()) {
        TLOG(TLVL_ERROR) << std::format("failed to open log file:{} in ios::app mode",log_fn); 
      }
      for (int i=0; i<ns; i++) {
        log_file << vs[i] << std::endl;
        TLOG(TLVL_DEBUG+1) << vs[i];
      }
      log_file.close();
    }
  }
  
  TLOG(TLVL_DEBUG) << "-- END"; 
  return 0;
}
