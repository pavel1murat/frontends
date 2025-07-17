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
#define  TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
// takes parameters from ODB
// a DTC can execute one command at a time
// this command is fast,
// equipment knows the node name and has an interface to ODB
//-----------------------------------------------------------------------------
int TEqTrkDtc::ConfigureJA(std::ostream& Stream) {

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
// takes parameters frpm ODB
//-----------------------------------------------------------------------------
int TEqTrkDtc::DigiRW(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_cmd = odb_i->GetDtcCommandHandle(HostLabel(),_dtc_i->PcieAddr());

  std::string cmd            = odb_i->GetString(h_cmd,"Name");
  std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
  
  HNDLE h_cmd_par            = odb_i->GetHandle(0,parameter_path);
    
  trkdaq::ControlRoc_DigiRW_Input_t  par;
  trkdaq::ControlRoc_DigiRW_Output_t pout;
    
  par.rw           = odb_i->GetInteger(h_cmd_par,"rw");         //
  par.hvcal        = odb_i->GetInteger(h_cmd_par,"hvcal");      //
  par.address      = odb_i->GetInteger(h_cmd_par,"address");    //
  par.data[0]      = odb_i->GetInteger(h_cmd_par,"data[0]"); //
  par.data[1]      = odb_i->GetInteger(h_cmd_par,"data[1]"); //

  int  print_level = odb_i->GetInteger(h_cmd_par,"print_level");
  int  link        = odb_i->GetInteger(h_cmd_par,"link");
   
  printf("dtc_i->fLinkMask: 0x%04x\n",_dtc_i->fLinkMask);
  _dtc_i->ControlRoc_DigiRW(&par,&pout,link,print_level,Stream);
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::DumpSettings(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/dump_settings");
    
  int link        = o_cmd["link"       ];    //
  int channel     = o_cmd["channel"    ];    //
  int print_level = o_cmd["print_level"];

  int lnk1(link), lnk2(link+1);
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue ;
    _dtc_i->ControlRoc_DumpSettings(lnk,channel,print_level,Stream);
  }
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::GetKey(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"get_key");

  int link               = odb_i->GetInteger(h_cmd_par,"link");

  try         {
    std::vector<uint16_t> data;
    _dtc_i->ControlRoc_GetKey(data,link,2,Stream);
  }
  catch (...) { Stream << "ERROR : coudn't execute ControlRoc_GetKey ... BAIL OUT" << std::endl; }
  
  return 0;
}


//-----------------------------------------------------------------------------
int TEqTrkDtc::GetRocDesignInfo(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"get_roc_design_info");

  int link               = odb_i->GetInteger(h_cmd_par,"link");

  int rmin(link), rmax(link+1);
  if (link == -1) {
    rmin = 0;
    rmax = 6;
  }
  
  TLOG(TLVL_INFO) << "-------------- rmin, rmax:" << rmin << " " << rmax;

  for (int i=rmin; i<rmax; ++i) {
    Stream << "----- link:" << i ;
    if (_dtc_i->LinkEnabled(i) == 0) {
      Stream << " disabled, continue" << std::endl;
      continue;
    }
      // ROC ID
    try         {
      std::string roc_id = _dtc_i->GetRocID(i);
      Stream << std::endl;
      Stream << "roc_id         :" << roc_id << std::endl;
    }
    catch (...) {
      Stream << "ERROR : coudn't execute GetRocID ... BAIL OUT" << std::endl;
    }
    // design info 
    try         {
      std::string design_info = _dtc_i->GetRocDesignInfo(i);
      Stream << "roc_design_info:" << design_info << std::endl;
    }
    catch (...) {
      Stream << "ERROR : coudn't execute GetRocDesignInfo ... BAIL OUT" << std::endl;
    }
      // git commit
    try {
      std::string git_commit = _dtc_i->GetRocFwGitCommit(i);
      Stream << "git_commit     :" << "'"  << git_commit << "'" << std::endl;
    }
    catch (...) {
      Stream << "ERROR : coudn't execute GetRocFwGitCommit ... BAIL OUT" << std::endl;
    }
  }

  return 0;
}


//-----------------------------------------------------------------------------
int TEqTrkDtc::InitReadout(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"init_readout");

  try {
    uint32_t emulate_cfo      = odb_i->GetUInt32(h_cmd_par,"emulate_cfo"     );
    uint32_t roc_readout_mode = odb_i->GetUInt32(h_cmd_par,"roc_readout_mode");
    
    _dtc_i->InitReadout(emulate_cfo,roc_readout_mode);

    Stream << "DTC:" << _dtc_i->PcieAddr() << " emulate_cfo:" << emulate_cfo
           << " roc_readout_mode:" << roc_readout_mode << " init readout OK";
  }
  catch (...) {
    Stream << "ERROR : coudn't init readout DTC:" << _dtc_i->PcieAddr();
  }
  
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::FindAlignment(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"find_alignment");

  int link        = odb_i->GetInteger(h_cmd_par,"link"       );
  // int print_level = odb_i->GetInteger(h_cmd_par,"print_level");

  try {
    _dtc_i->FindAlignments(1,link,Stream);
  }
  catch (...) {
    Stream << " -- ERROR : coudn't execute FindAlignments for link:" << link << " ... BAIL OUT" << std::endl;
  }
  
  return 0;
}

//-----------------------------------------------------------------------------
// link=-1: print status of all enabled ROCs
//-----------------------------------------------------------------------------
int TEqTrkDtc::PrintRocStatus(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"print_roc_status");

  int link        = odb_i->GetInteger(h_cmd_par,"link"       );
  try         {
    _dtc_i->PrintRocStatus(1,link,Stream);
  }
  catch (...) {
    Stream << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::PulserOff(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"pulser_off");

  int link, print_level;

  link               = odb_i->GetInteger(h_cmd_par,"link"              );    //
  print_level        = odb_i->GetInteger(h_cmd_par,"print_level"       );

  TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOff, link:" << link;
  try {
    _dtc_i->ControlRoc_PulserOff(link,print_level,Stream);
  }
  catch(...) {
    Stream << "ERROR : coudn't execute ControlRoc_PulserOFF ... BAIL OUT" << std::endl;
  }
  
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::PulserOn(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"pulser_on");

  int link, first_channel_mask, duty_cycle, pulser_delay, print_level;

  link               = odb_i->GetInteger(h_cmd_par,"link"              );    //
  first_channel_mask = odb_i->GetInteger(h_cmd_par,"first_channel_mask");    //
  duty_cycle         = odb_i->GetInteger(h_cmd_par,"duty_cycle"        );    //
  pulser_delay       = odb_i->GetInteger(h_cmd_par,"pulser_delay"      );    //
  print_level        = odb_i->GetInteger(h_cmd_par,"print_level"       );

  TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOn, link:" << link;
  try {
    _dtc_i->ControlRoc_PulserOn(link,first_channel_mask,duty_cycle,pulser_delay,print_level,Stream);
  }
  catch(...) {
    Stream << " -- ERROR : coudn\'t execute ControlRoc_PulserOn ... BAIL OUT" << std::endl;
  }
  
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");

  try {
    int      timeout_ms(150);
    uint32_t reg = odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val;
    _dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
    odb_i->SetUInt32(h_cmd_par,"Value",val);
    Stream << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) { Stream << " ERROR : dtc_read_register ... BAIL OUT" << std::endl; }

  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadRocRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_roc_register");

  int link        = odb_i->GetInteger(h_cmd_par,"link"    );
  uint16_t reg    = odb_i->GetInteger(h_cmd_par,"register");
//-----------------------------------------------------------------------------
// ROC registers are 16-bit
//-----------------------------------------------------------------------------
    try {
      int timeout_ms(150);
      uint16_t val = _dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(link),reg,timeout_ms);
      odb_i->SetInteger(h_cmd_par,"value",val);

      Stream << " -- read_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) {
      Stream << " -- ERROR : coudn't read ROC register ... BAIL OUT";
    }
  
  return 0;
}


//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadIlp(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_ilp");

  int link         = o["link"       ];
  int print_level  = o["print_level"];
  
  try         {
    std::vector<uint16_t>   data;
    _dtc_i->ControlRoc_ReadIlp(data,link,print_level,Stream);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "failed ControlRoc_ReadIlp for link:" << link;
    Stream << "ERROR : coudn't read ILP ... BAIL OUT" << std::endl;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadSpi(std::ostream& Stream) {
  int rc(0);

  OdbInterface* odb_i        = OdbInterface::Instance();
  HNDLE h_cmd                = odb_i->GetDtcCommandHandle(HostLabel(),_dtc_i->PcieAddr());
  std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
  HNDLE h_cmd_par            = odb_i->GetHandle(h_cmd,parameter_path);

  int link         = odb_i->GetInteger(h_cmd_par,"link"       ); // o["link"       ];
  int print_level  = odb_i->GetInteger(h_cmd_par,"print_level"); // o["print_level"];
  
  try         {
    if (link != -1) {
      std::vector<uint16_t>   data;
      _dtc_i->ControlRoc_ReadSpi(data,link,print_level,Stream);
    }
    else {
                                        // need formatted printout for all ROCs
      trkdaq::TrkSpiData_t spi[6];
      for (int i=0; i<6; i++) {
        if (_dtc_i->LinkEnabled(i)) {
          _dtc_i->ControlRoc_ReadSpi_1(&spi[i],i,0,Stream);
        }
      }
                                        // now - printing
      _dtc_i->PrintSpiAll(spi,Stream);
    }
  }
  catch (...) {
    Stream << "ERROR : coudn't read SPI ... BAIL OUT" << std::endl;
    rc = -1;
  }

  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadDDR(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_ddr");

  int block_number = o["block_number"];
  int link         = o["link"        ];
  // int print_level  = o["print_level"];
  
  try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
    _dtc_i->ReadRocDDR(link,block_number,Stream);
  }
  catch(...) {
    TLOG(TLVL_ERROR) << "failed ControlRoc_ReadDDR for link:" << link;
    Stream << "ERROR : coudn't execute ControlRoc_ReadDDR ... BAIL OUT" << " link:" << link << std::endl;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ResetRoc(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/reset_roc");

  int link         = o["link"       ];
  int print_level  = o["print_level"];
  
  rc = _dtc_i->ResetLink(link);

  if (rc == 0) Stream << " -- reset_roc OK";
  else         Stream << " -- ERROR: failed reset_roc link:" << link << std::endl;
  
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::SetCalDac(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/set_caldac");
    
  int first_channel_mask = o_cmd["first_channel_mask"];    //
  int value              = o_cmd["value"             ];    //
  int print_level        = o_cmd["print_level"       ];
  int link               = o_cmd["link"              ];

  int lnk1(link), lnk2(link+1);
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue ;
    _dtc_i->ControlRoc_SetCalDac(lnk,first_channel_mask,value,print_level,Stream);
  }
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::WriteRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");

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
int TEqTrkDtc::WriteRocRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"write_roc_register");

  int      link = odb_i->GetInteger(h_cmd_par,"link"    );
  uint16_t reg  = odb_i->GetInteger(h_cmd_par,"register");
  uint16_t val  = odb_i->GetInteger(h_cmd_par,"value");
//-----------------------------------------------------------------------------
// ROCs have 16-bit registers
//-----------------------------------------------------------------------------
  try {
    int timeout_ms(150);
    _dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(link),reg,val,false,timeout_ms);
    Stream << " -- write_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    Stream << "ERROR : coudn't write ROC register ... BAIL OUT" << std::endl;
  }

  return 0;
}


