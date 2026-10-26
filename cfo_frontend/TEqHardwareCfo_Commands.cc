/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include <chrono>
// #include <fmt/chrono.h>

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

  rc = _cfo_i->ConfigureJA(-1,-1,sstr);

  sstr << std::format("ConfigureJA done, rc:{}",rc);

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
int TEqHardwareCfo::GetTimingChainTopology() {
  int rc(0);
  TLOG(TLVL_DEBUG) << "-- START";
  // initializes _timing_chain
  
  TLOG(TLVL_DEBUG) << std::format("-- END : rc:{}",rc);
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
// from the DTC configuration
// run plan alwys comes from the CFO ODB configuration - can change on the fly
//-----------------------------------------------------------------------------
int TEqHardwareCfo::InitTimingChain(HNDLE H_Cmd) {
  int rc(0);

  TLOG(TLVL_DEBUG) << std::format("-- START: H_Cmd:{}",H_Cmd);
  SetStatus(1); 

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
 
  std::string cmd               = _odb_i->GetString (H_Cmd,"Name" );   // better be "init_timing_chain"
  std::string logfile           = _odb_i->GetString (H_Cmd,"logfile" );
  std::string run_plan_dir      = _odb_i->GetCfoRunPlanDir();
  std::string run_plan          = _odb_i->GetString (_handle,"run_plan"         );
  //  int         timing_chain_mask = _odb_i->GetUInt32 (_handle,"timing_chain_mask");

  // std::string run_plan_fn = run_plan_dir+'/'+run_plan+".bin";
  // TLOG(TLVL_DEBUG) << std::format("run_plan_fn:{}",run_plan_fn);
  
  std::string cmd_parameter_path = _odb_i->GetCfoCmdParameterPath(cmd);
  // sstr << std::endl;

  // 1. get topology of the timing chain from ODB
  GetTimingChainTopology();
  
  // 2. initial: process the chains sequentially, initialize tthe DTC's in the right order
  // a chain is a vector of strings : 'nodename:dtci'
  // the whole thing is a vector of timing chains

  //  int print_level = _odb_i->GetInteger(H_Cmd,"print_level");

  for (int i=0; i<8; i++) {
    if (not _cfo_i->LinkEnabled(i)) continue;
    std::vector<time_chain_node_t>* chain = &_timing_chain[i];
    
    for (auto chain_node : *chain) {
      // split node and dtc id, for example
      std::string node = "aaaaa";
      std::string pcie_addr  = "0" ;
      // send a 'fix_cfo_edge" ("setup_cfo_link") command to the DTC
      std::string dtc_cmd_path = std::format("/Mu2e/Commands/DAQ/Nodes/{}/DTC{}",node,pcie_addr);
      HNDLE h_dtc_cmd          = _odb_i->GetHandle(0,dtc_cmd_path);
      // 
      // HNDLE h_cmd              = _odb_i->GetHandle(h_dtc_cmd,"setup_cfo_link");
      
      _odb_i->SetString (h_dtc_cmd,"logfile"      ,"default");
      //      _odb_i->SetInteger(h_dtc_cmd,"link"         ,lnk); // don't need links
      _odb_i->SetString (h_dtc_cmd,"ParameterPath",cmd_parameter_path);
      _odb_i->SetInteger(h_dtc_cmd,"Finished"     , 0);
      _odb_i->SetInteger(h_dtc_cmd,"ReturnCode"   , 0);
//-----------------------------------------------------------------------------
// and trigger the execution of a 'per-DTC' command
//-----------------------------------------------------------------------------
        _odb_i->SetInteger(h_dtc_cmd,"Run"          , 1);
    }
  }
  
  // at this point need to go into a wait mode
  
  rc = WaitForCompletion(H_Cmd);
                                     
  if (rc < 0) {
    std::string msg = std::format("failed to initialize the CFO timing chain");
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
    sstr << "soft reset OK" << std::endl;
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

    sstr << "write_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << "ERROR : dtc_write_register ... BAIL OUT" << std::endl;
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

  std::string cmd  = _odb_i->GetString (h_Cmd,"Name");
  
  auto now = std::chrono::system_clock::now();

  std::time_t t = std::chrono::system_clock::to_time_t(now);
  char buf[20];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  
  // Manually append the 2-digit milliseconds
  std::string s_now = std::format("{}.{:02}", buf, ms.count() / 10);

  // auto now = std::chrono::system_clock::now();
  // // This works perfectly in {fmt} even where std::format fails ... with that was true
  // std::string s_now = fmt::format("{:%Y-%m-%d %H:%M:%.2S}", now);
  
  Stream << std::format("{} CFO cmd:{} label:{} pcie_addr:{}\n",s_now,cmd,HostLabel(),_cfo_i->PcieAddr());
  return 0;
}


//-----------------------------------------------------------------------------
// this is mot a static function
//-----------------------------------------------------------------------------
int TEqHardwareCfo::WaitForCompletion(HNDLE H_Cmd) {
  int return_code(0);
  TLOG(TLVL_DEBUG) << std::format("--- START: hCmd:{}",H_Cmd);

  HNDLE h_cmd_par     = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string cmd     = _odb_i->GetString (H_Cmd    ,"Name"      );
  std::string logfile = _odb_i->GetString (H_Cmd    ,"logfile"   );
  int timeout_ms      = _odb_i->GetInteger(h_cmd_par,"timeout_ms");

  TLOG(TLVL_DEBUG) << std::format("cmd:{} timeout_ms:{}",cmd,timeout_ms);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  int wait_time_ms(0), n_not_finished(100);

  while ((n_not_finished > 0) and (wait_time_ms < timeout_ms)) {
    ss_sleep(100);
    wait_time_ms  += 100;
    n_not_finished = 0;
    
    for (int i=0; i<8; i++) {
      if (not _cfo_i->LinkEnabled(i)) continue;
      std::vector<time_chain_node_t>* chain = &_timing_chain[i];

      for (auto chain_node : *chain) {

        if (chain_node.finished == 1) continue;
      
        // split node and dtc id, for example
        std::string node      = "aaaaa";
        std::string pcie_addr = "0" ;
        // send a 'fix_cfo_edge" ("setup_cfo_link") command to the DTC
        std::string dtc_cmd_path = std::format("/Mu2e/Commands/DAQ/Nodes/{}/DTC{}",node,pcie_addr);
        HNDLE h_dtc_cmd          = _odb_i->GetHandle(0,dtc_cmd_path);

        chain_node.finished = _odb_i->GetInteger(h_dtc_cmd,"Finished");
        if (chain_node.finished) {
          int rcc = _odb_i->GetInteger(h_dtc_cmd,"ReturnCode");
          if (rcc < 0) {
            return_code  += rcc;
          }
        }
        else {
          n_not_finished += 1;
        }
      }
    }
  }

  if (return_code == 0) {
    if (n_not_finished > 0) return_code = 1; // (BUSY) ... timeout
  }
  
  
  std::string msg = std::format("n_not_finished:{} return_code:{}",n_not_finished,return_code);
  sstr << msg << std::endl;
  TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);


//-----------------------------------------------------------------------------
// until the error handling is understood, always set status to zero
//-----------------------------------------------------------------------------
  SetCommandFinished(H_Cmd,0); // return_code);

  TLOG(TLVL_DEBUG) << std::format("-- END: {}",msg);
  return return_code;
}
