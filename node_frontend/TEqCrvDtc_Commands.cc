/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
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
// takes parameters from ODB
// a DTC can execute one command at a time
// this command is fast,
// equipment knows the node name and has an interface to ODB
//-----------------------------------------------------------------------------
int TEqCrvDtc::ConfigureJA(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";

  // OdbInterface* odb_i    = OdbInterface::Instance();
  std::string   cmd_path = std::format("/Mu2e/Commands/DAQ/Nodes/{}/DTC{}",_host_label,_dtc_i->PcieAddr());

  midas::odb o_cmd(cmd_path.data());

  std::string parameter_path = o_cmd["ParameterPath"];
  o_cmd["Finished"] = 0;
  
  //  std::string cmd_path = path+"/configure_ja";
  // int print_level      = o_cmd["print_level"];

  int rc = _dtc_i->ConfigureJA();

  o_cmd["ReturnCode"] = rc;
  o_cmd["Run"       ] =  0;
  o_cmd["Finished"  ] =  1;
  
  TLOG(TLVL_DEBUG) << "--- END : rc:" << rc;
  return 0;
}

//-----------------------------------------------------------------------------
// 'roc_readout_mode' should be taken from the DAQ readout configuration
// 'emulate_cfo'      - from the DTC configuration
//-----------------------------------------------------------------------------
int TEqCrvDtc::InitReadout(std::ostream& Stream) {
  int rc(0);

  TLOG(TLVL_DEBUG) << "-- START";
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_dtc     = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_daq     = odb_i->GetDaqConfigHandle();
  
  uint32_t roc_readout_mode = odb_i->GetInteger(h_daq,"RocReadoutMode");
  uint32_t roc_lane_mask    = odb_i->GetUInt32(h_dtc,"RocLaneMask");

  TLOG(TLVL_DEBUG) << "roc_readout_mode:" << roc_readout_mode
                   << " roc_lane_mask:0x" << std::hex << roc_lane_mask;

  try {
    rc = _dtc_i->InitReadout(-1,roc_readout_mode);

    Stream << " emulate_cfo:" << _dtc_i->EmulateCfo()
           << " roc_readout_mode:" << roc_readout_mode << " rc:" << rc;
  }
  catch (...) {
    Stream << "ERROR : coudn't init DTC readout";
  }
  
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;
  return rc;
}

//-----------------------------------------------------------------------------
// link=-1: print status of all enabled ROCs
//-----------------------------------------------------------------------------
int TEqCrvDtc::PrintRocStatus(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());

  int link        = odb_i->GetInteger(h_cmd,"link"       );
  try         {
    _dtc_i->PrintRocStatus(1,link,Stream);
  }
  catch (...) {
    Stream << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");

  try {
    int      timeout_ms(150);
    uint32_t reg = odb_i->GetUInt32(h_cmd_par,"register");
    uint32_t val;
    _dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
    odb_i->SetUInt32(h_cmd_par,"value",val);
    Stream << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) { Stream << " ERROR : dtc_read_register ... BAIL OUT" << std::endl; }

  return 0;
}

//-----------------------------------------------------------------------------
// assume that link != -1 (read only one ROC), thus don't inject '\n'
// the rest is printed by _ProcessComand
//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadRocRegister(std::ostream& Stream) {
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link        = odb_i->GetInteger(h_cmd    ,"link"    );
  uint16_t reg    = odb_i->GetUInt16 (h_cmd_par,"register");
  TLOG(TLVL_DEBUG) << "link:" << link << " reg:" << reg;
//-----------------------------------------------------------------------------
// ROC registers are 16-bit
//-----------------------------------------------------------------------------
  try {
    int timeout_ms(150);
    uint16_t val = _dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(link),reg,timeout_ms);
    odb_i->SetUInt16(h_cmd_par,"value",val);
    
    Stream << " reg:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    Stream << " -- ERROR : coudn't read ROC register:0x" << std::hex << reg << " ... BAIL OUT";
  }
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::ResetRoc(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/reset_roc");

  HNDLE         h_cmd     = _odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string   cmd_name  = _odb_i->GetString(h_cmd,"Name");

  int link         = _odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];

  //  int print_level  = o["print_level"];
  
  rc = _dtc_i->ResetLink(link);

  if (rc == 0) Stream << " -- reset_roc OK";
  else         Stream << " -- ERROR: failed reset_roc link:" << link << " rc:" << rc << std::endl;
  
  return rc;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::WriteRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  try {
    int      timeout_ms(150);
    uint32_t reg = odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val = odb_i->GetUInt32(h_cmd_par,"Value"   );
    _dtc_i->fDtc->GetDevice()->write_register(reg,timeout_ms,val);

    Stream << " -- write_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    Stream << " ERROR : dtc_write_register ... BAIL OUT" << std::endl;
  }

  return 0;
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::WriteRocRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int      link = odb_i->GetInteger(h_cmd    ,"link"    );

  uint16_t reg  = odb_i->GetUInt16 (h_cmd_par,"register");
  uint16_t val  = odb_i->GetUInt16 (h_cmd_par,"value"   );
//-----------------------------------------------------------------------------
// ROCs have 16-bit registers
//-----------------------------------------------------------------------------
  try {
    int timeout_ms(150);
    _dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(link),reg,val,false,timeout_ms);
    Stream << " register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    Stream << " ERROR : coudn't write ROC register:" << std::hex << reg << " ... BAIL OUT" << std::endl;
  }

  return 0;
}


