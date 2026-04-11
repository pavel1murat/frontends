/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "cfo_frontend/TEqHardwareCfo.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"
#include <regex>

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqHardwareCfo"

//-----------------------------------------------------------------------------
// takes parameters from ODB
// a DTC can execute one command at a time
// this command is fast,
// equipment knows the node name and has an interface to ODB
// H_Cmd should point to "/Mu2e/Commands/DAQ/CFO"
//-----------------------------------------------------------------------------
int TEqHardwareCfo::ConfigureJA(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: H_Cmd:{}",H_Cmd);
  SetStatus(1);

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  HNDLE       h_cmd     = _odb_i->GetCfoCmdHandle(_h_active_run_conf);
  // HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);

  std::string logfile   = _odb_i->GetString (h_cmd    ,"logfile"    );

  // int         clock_source = _odb_i->GetInteger(h_cmd_par,"clock_source");
  // int         reset        = _odb_i->GetInteger(h_cmd_par,"reset"       );

  rc = _cfo_i->ConfigureJA(sstr);

  sstr << std::format(" ConfigureJA done, rc:{}",rc);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{} cmd_rc:{}",rc,cmd_rc);
  
  return 0;
}

//-----------------------------------------------------------------------------
int TEqHardwareCfo::CompileRunPlan(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: H_Cmd:{}",H_Cmd);
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  HNDLE       h_cmd        = _odb_i->GetCfoCmdHandle(_h_active_run_conf);
  HNDLE       h_cmd_par    = _odb_i->GetCmdParameterHandle(H_Cmd);

  std::string logfile      = _odb_i->GetString (h_cmd    ,"logfile"    );
  int         print_level  = _odb_i->GetInteger(h_cmd_par,"print_level");
  std::string run_plan_dir = _odb_i->GetCfoRunPlanDir();
  std::string run_plan     = _odb_i->GetCfoRunPlan(_handle);

                                        // GetRunPlan returns a name stub, make a full file name out of that
                                        // convention : ".txt" --> ".bin"
  
  std::string input_fn  = run_plan_dir+"/"+run_plan+".txt";
  std::string output_fn = run_plan_dir+"/"+run_plan+".bin";
 
  TLOG(TLVL_DEBUG) << std::format("input_fn:{} output_fn:{}",input_fn,output_fn);
  
  _cfo_i->CompileRunPlan(input_fn,output_fn,print_level,sstr);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// HALT execution of run plan
//-----------------------------------------------------------------------------
int TEqHardwareCfo::Halt(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  std::string logfile      = _odb_i->GetString (H_Cmd    ,"logfile"    );

  rc = _cfo_i->Halt();

  sstr << std::format(" halt: rc:{}",rc);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc); 
  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{} cmd_rc:{}",rc,cmd_rc);

  return rc;
}

//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
int TEqHardwareCfo::HardReset(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );

  try         {
    _cfo_i->Cfo()->HardReset();
    sstr << " hard reset OK" << std::endl;
  }
  catch (...) {
    sstr << "ERROR : coudn't hard reset the CFO ... BAIL OUT" << std::endl;
  }
                                        // logfile is not qualified with the path
                                        // this is transition !
  
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
// from the DTC configuration
// run plan alwys comes from the CFO ODB configuration - can change on the fly
//-----------------------------------------------------------------------------
int TEqHardwareCfo::InitReadout(HNDLE H_Cmd) {
  int rc(0);

  TLOG(TLVL_DEBUG) << std::format("-- START: H_Cmd:{}",H_Cmd);
  SetStatus(1); 

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
 
  std::string logfile           = _odb_i->GetString (H_Cmd,"logfile" );
  std::string run_plan_dir      = _odb_i->GetCfoRunPlanDir();
  std::string run_plan          = _odb_i->GetString (_handle,"run_plan"         );
  int         timing_chain_mask = _odb_i->GetUInt32 (_handle,"timing_chain_mask");

  std::string run_plan_fn = run_plan_dir+'/'+run_plan+".bin";
  TLOG(TLVL_DEBUG) << std::format("run_plan_fn:{}",run_plan_fn);
  
  sstr << std::endl;

  rc = _cfo_i->InitReadout(run_plan_fn.data(),timing_chain_mask,sstr);

  sstr << std::format(" run_plan_fn:{} time_chain_mask:0x{:08x} rc:{}",run_plan_fn,timing_chain_mask,rc);

  if (rc < 0) {
    std::string msg = std::format("failed to initialize the CFO readout for run_plan_fn:{}",run_plan_fn);
    cm_msg(MERROR,__func__,msg.data());
    cm_msg_flush_buffer();
    TLOG(TLVL_ERROR) << msg;
  }
 
  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,cmd_rc);
  
  return rc;
}

//-----------------------------------------------------------------------------
// launch already compiled and loaded run plan
//-----------------------------------------------------------------------------
int TEqHardwareCfo::LaunchRunPlan(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  //   HNDLE       h_cmd_par    = _odb_i->GetCmdParameterHandle(H_Cmd);

  std::string logfile      = _odb_i->GetString (H_Cmd    ,"logfile"    );
  // int         print_level  = _odb_i->GetInteger(h_cmd_par,"print_level");

  _cfo_i->LaunchRunPlan();

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc);
  
  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// print DTC status
//-----------------------------------------------------------------------------
int TEqHardwareCfo::PrintStatus(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  // HNDLE       h_cmd_par   = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );

  TLOG(TLVL_DEBUG) << std::format("logfile:{}",logfile);
  _cfo_i->PrintStatus(sstr);
                                        // logfile is not qualified with the path
                                        // this is transition ! 
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
int TEqHardwareCfo::ReadRegister(HNDLE H_Cmd) {
  int rc(0);
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
 
  HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile   = _odb_i->GetString (H_Cmd    ,"logfile"    );

  try {
    int      timeout_ms(150);
    uint32_t reg = _odb_i->GetUInt32(h_cmd_par,"register");
    uint32_t val;
    _cfo_i->fCfo->GetDevice()->read_register(reg,timeout_ms,&val);
    _odb_i->SetUInt32(h_cmd_par,"value",val);
    sstr << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " ERROR : dtc_read_register ... BAIL OUT" << std::endl;
  }

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// load into the FPGA memory already compiled run plan - to avoid ambiguities,
// use the one defined in the CFO configuration ??? 
//-----------------------------------------------------------------------------
int TEqHardwareCfo::SetRunPlan(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  HNDLE       h_cmd_par    = _odb_i->GetCmdParameterHandle(H_Cmd);

  std::string logfile      = _odb_i->GetString (H_Cmd    ,"logfile" );
  std::string run_plan_dir = _odb_i->GetCfoRunPlanDir();
  std::string run_plan     = _odb_i->GetString (h_cmd_par,"run_plan");

  std::string run_plan_fn  = run_plan_dir+'/'+run_plan+".bin";
  TLOG(TLVL_DEBUG) << std::format("run_plan_fn:{}",run_plan_fn);

  _cfo_i->SetRunPlan(run_plan_fn);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{} cmd_rc:{}",rc,cmd_rc);

  return rc;
}

//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
int TEqHardwareCfo::SoftReset(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );

  try         {
    _cfo_i->Cfo()->SoftReset();
    sstr << " soft reset OK" << std::endl;
  }
  catch (...) {
    sstr << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl;
  }
                                        // logfile is not qualified with the path
                                        // this is transition !
  
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
int TEqHardwareCfo::WriteRegister(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  HNDLE h_cmd_par     = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile = _odb_i->GetString (H_Cmd    ,"logfile"    );

  try {
    int      timeout_ms(150);
    uint32_t reg = _odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val = _odb_i->GetUInt32(h_cmd_par,"Value"   );
    _cfo_i->fCfo->GetDevice()->write_register(reg,timeout_ms,val);

    sstr << " -- write_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " ERROR : dtc_write_register ... BAIL OUT" << std::endl;
  }

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// for single-link commands aim for a 1 line output,
// for 'all-active-link' comamnds (link=-1) print the header and then - 
// one line per link
//-----------------------------------------------------------------------------
int TEqHardwareCfo::StartMessage(HNDLE h_Cmd, std::stringstream& Stream) {

  Stream << std::endl; // perhaps

  std::string cmd  = _odb_i->GetString (h_Cmd,"Name");
  
  Stream << std::format("-- label:{} host:{} cmd:{} pcie_addr:{}\n",
                        HostLabel(),FullHostName(),cmd,_cfo_i->PcieAddr());
  return 0;
}
