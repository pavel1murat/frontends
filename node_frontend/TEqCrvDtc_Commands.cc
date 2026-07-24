/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include <chrono>

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqCrvDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCrvDtc"

//-----------------------------------------------------------------------------
// clear DTC status in ODB (including status oof its links)
//-----------------------------------------------------------------------------
int TEqCrvDtc::ClearStatus(HNDLE H_Cmd) { // std::ostream& Stream) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "--- START";
  SetStatus(1); // BUSY
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  // HNDLE h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  
  std::string logfile = _odb_i->GetString (H_Cmd,"logfile");
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  for (int i=0; i<6; i++) {
    _dtc_i->ClearLinkStatus(i);
                                        // and reflect that in ODB
    std::string link_odb_path = std::format("Link{}/Status",i);
    _odb_i->SetInteger(_handle,link_odb_path.data(),0);
  }
                                        // and clear own status
  SetStatus(0);

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc);
  
  TLOG(TLVL_DEBUG) << std::format("--- END rc:{} log_rc:{}",rc,log_rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::ConfigureJA(HNDLE H_Cmd) { // std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  SetStatus(1); // BUSY
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  // HNDLE h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  
  std::string logfile = _odb_i->GetString (H_Cmd,"logfile");
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  // use settings from ODB read out in the constructor, but redirect the output
  int rc = _dtc_i->ConfigureJA(-1,-1,sstr);

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc);
  
  TLOG(TLVL_DEBUG) << std::format("--- END rc:{} log_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
// 'roc_readout_mode' should be taken from the DAQ readout configuration
// 'emulate_cfo'      - from the DTC configuration
//-----------------------------------------------------------------------------
int TEqCrvDtc::InitReadout(HNDLE H_Cmd) { // std::ostream& Stream) {
  int rc(0);

  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  std::string logfile             = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  HNDLE    h_daq                  = _odb_i->GetDaqConfigHandle();

  uint32_t roc_readout_mode       = _odb_i->GetInteger(h_daq,"RocReadoutMode"        );

  TLOG(TLVL_DEBUG) << std::format("roc_readout_mode:{}",roc_readout_mode);

                                                       // soft_reset(DTC) + init_cfo + reset_rocs + init_roc_readout_mode + release_buffers
  rc = _dtc_i->InitReadout(-1,roc_readout_mode,sstr);
  if (rc < 0) {
    TLOG(TLVL_ERROR) << std::format("node:{} DTC:{} : failed to initialize the DTC readout. BAIL OUT",
                                    HostLabel(),_dtc_i->PcieAddr());
    
    int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
    SetCommandFinished(H_Cmd,rc);
    TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
    return rc;
  }
  
  if (rc == 0) {
    sstr << std::format(" emulate_cfo:{} roc_readout_mode:{} rc:{}",_dtc_i->EmulateCfo(),roc_readout_mode,rc);
  }
  else {
    std::string msg = std::format("host:{} DTC:{} coudn't init readout",HostLabel(),_dtc_i->PcieAddr());
    sstr << "ERROR:" << msg << "\n";
    TLOG(TLVL_ERROR) << msg;
    cm_msg(MERROR,__func__,msg.data());
    cm_msg_flush_buffer();
  }
  
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}


//-----------------------------------------------------------------------------
// HARD RESET
//-----------------------------------------------------------------------------
int TEqCrvDtc::HardReset(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  // HNDLE       h_cmd_par   = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  try         {
    _dtc_i->Dtc()->HardReset();
    sstr << " hard reset OK" << std::endl;
  }
  catch (...) {
    sstr << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl;
  }
                                        // logfile is not qualified with the path
                                        // this is transition !
  
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,log_rc);
  return 0;
}

//-----------------------------------------------------------------------------
// link=-1: print status of all enabled ROCs
//-----------------------------------------------------------------------------
int TEqCrvDtc::PrintRocStatus(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  // HNDLE       h_cmd_par   = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }
  int         link        = _odb_i->GetInteger(H_Cmd,"link"    );

  TLOG(TLVL_DEBUG) << std::format("link:{} logfile:{}",link,logfile);
  try         {
    rc = _dtc_i->PrintRocStatus(1,link,sstr);
  }
  catch (...) {
    sstr << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl;
  }
                                        // logfile is not qualified with the path
  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,cmd_rc);
  return 0;
}

//-----------------------------------------------------------------------------
// print DTC status
//-----------------------------------------------------------------------------
int TEqCrvDtc::PrintStatus(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  // HNDLE       h_cmd_par   = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  TLOG(TLVL_DEBUG) << std::format("logfile:{}",logfile);
  rc = _dtc_i->PrintStatus(sstr);
                                        // logfile is not qualified with the path
                                        // this is transition ! 
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return 0;
}


//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadRegister(HNDLE H_Cmd) {
  int rc(0);

  TLOG(TLVL_DEBUG) << std::format("-- START:");
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  std::string logfile   = _odb_i->GetString(H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  HNDLE       h_cmd_par = _odb_i->GetHandle(H_Cmd,"read_register");

  uint32_t reg(0);
  try {
    int      timeout_ms(150);
    reg = _odb_i->GetUInt32(h_cmd_par,"register");
    uint32_t val;
    _dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
    _odb_i->SetUInt32(h_cmd_par,"value",val);
    sstr << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " ERROR : dtc_read_register ... BAIL OUT" << std::endl;
    TLOG(TLVL_ERROR) << std::format("failed to read PCIE:{} register:0x{:04x}",_dtc_i->PcieAddr(),reg);
  }

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc); 
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// assume that link != -1 (read only one ROC), thus don't inject '\n'
// the rest is printed by _ProcessComand
//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadRocRegister(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  int         link      = _odb_i->GetInteger(H_Cmd,"link"   );
  std::string logfile   = _odb_i->GetString (H_Cmd,"logfile");
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }
  
  HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);

  uint16_t reg    = _odb_i->GetUInt16 (h_cmd_par,"register");
  TLOG(TLVL_DEBUG) << "link:" << link << " reg:" << reg;
//-----------------------------------------------------------------------------
// ROC registers are 16-bit
//-----------------------------------------------------------------------------
  try {
    int timeout_ms(150);
    uint16_t val = _dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(link),reg,timeout_ms);
    _odb_i->SetUInt16(h_cmd_par,"value",val);
    
    sstr << " reg:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " -- ERROR : coudn't read ROC register:0x" << std::hex << reg << " ... BAIL OUT";
  }

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadSubevents(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  HNDLE       h_cmd_par   = _odb_i->GetCmdParameterHandle(H_Cmd);
  
  std::string logfile     = _odb_i->GetString (H_Cmd    ,"logfile"    );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }
  int         first_ewt   = _odb_i->GetInteger(h_cmd_par,"first_ewt"  );
  int         print_level = _odb_i->GetInteger(h_cmd_par,"print_level");
  int         validate    = _odb_i->GetInteger(h_cmd_par,"validate"   );
  std::string output_fn   = _odb_i->GetString (h_cmd_par,"output_fn"  );
  
  TLOG(TLVL_DEBUG) << std::format("print_level:{} logfile:{}",print_level,logfile);
//-----------------------------------------------------------------------------
// write output to the DTC log
//-----------------------------------------------------------------------------
  std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> list_of_subevents;

  _dtc_i->ReadSubevents(list_of_subevents,first_ewt,print_level,sstr,validate,output_fn);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}


//-----------------------------------------------------------------------------
int TEqCrvDtc::ResetRoc(HNDLE H_Cmd) {
  int rc(0);
  //   midas::odb o   ("/Mu2e/Commands/Tracker/DTC/reset_roc");

  TLOG(TLVL_DEBUG) << std::format("-- START:");
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  int         link     = _odb_i->GetInteger(H_Cmd,"link"   ); // o["link"       ];
  std::string logfile  = _odb_i->GetString (H_Cmd,"logfile");
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  //  int print_level  = o["print_level"];
  
  rc = _dtc_i->ResetLink(link);

  if (rc == 0) sstr << " -- reset_roc OK";
  else         sstr << " -- ERROR: failed reset_roc link:" << link << " rc:" << rc << std::endl;
  
  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 

  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}



//-----------------------------------------------------------------------------
// SOFT RESET
//-----------------------------------------------------------------------------
int TEqCrvDtc::SoftReset(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  SetStatus(1);                         // BUSY

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);
  
  std::string logfile     = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  try         {
    _dtc_i->Dtc()->SoftReset();
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
// for single-link commands aim for a 1 line output,
// for 'all-active-link' comamnds (link=-1) print the header and then - 
// one line per link
//-----------------------------------------------------------------------------
int TEqCrvDtc::StartMessage(HNDLE h_Cmd, std::stringstream& Stream) {

  std::string cmd  = _odb_i->GetString (h_Cmd,"Name");
  int link         = _odb_i->GetInteger(h_Cmd,"link");
  
  auto now = std::chrono::system_clock::now();
    
  // // {:%Y-%m-%d %H:%M:%S} uses standard strftime-style flags
  // std::string s_now = std::format("{:%Y-%m-%d %H:%M:%S}", now);

  std::time_t t = std::chrono::system_clock::to_time_t(now);
  char buf[20];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  
  // Manually append the 2-digit milliseconds
  std::string s_now = std::format("{}.{:02}", buf, ms.count() / 10);

  // add message tag
  Stream << std::format("<msg> {} - cmd:{} label:{} pcie_addr:{} link:{}",s_now,cmd,HostLabel(),_dtc_i->PcieAddr(),link);
  Stream << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::WriteRegister(HNDLE H_Cmd) { 
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("-- START");
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  std::string logfile   = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  try {
    int      timeout_ms(150);
    uint32_t reg = _odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val = _odb_i->GetUInt32(h_cmd_par,"Value"   );
    _dtc_i->fDtc->GetDevice()->write_register(reg,timeout_ms,val);

    sstr << " -- write_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " ERROR : dtc_write_register ... BAIL OUT" << std::endl;
  }

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);
  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::WriteRocRegister(HNDLE H_Cmd) { // std::ostream& Stream) {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("-- START:");
  SetStatus(1);
  
  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  SetStatus(1); 

  HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  
  int         link      = _odb_i->GetInteger(H_Cmd,"link"    );
  std::string logfile   = _odb_i->GetString (H_Cmd,"logfile" );
  if (logfile == "default") {
    logfile = std::format("{}_dtc{}",HostLabel(),_dtc_i->PcieAddr());
  }

  uint16_t    reg       = _odb_i->GetUInt16 (h_cmd_par,"register");
  uint16_t    val       = _odb_i->GetUInt16 (h_cmd_par,"value"   );
//-----------------------------------------------------------------------------
// ROCs have 16-bit registers
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << std::format("link:{} reg:0x{:04x} val:0x{:04x}",link,reg,val);
  try {
    int timeout_ms(150);
    _dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(link),reg,val,false,timeout_ms);
    sstr << " reg:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    sstr << " ERROR : coudn't write ROC register:" << std::hex << reg << " ... BAIL OUT" << std::endl;
  }

  int log_rc = TMu2eEqBase::WriteOutput(sstr.str(),logfile,1);

  SetCommandFinished(H_Cmd,rc); 
  
  TLOG(TLVL_DEBUG) << std::format("-- END; rc:{} log_rc:{}",rc,log_rc);
  return rc;
}
