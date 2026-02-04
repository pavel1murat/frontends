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

  HNDLE h_cmd = odb_i->GetDtcCmdHandle(HostLabel(),_dtc_i->PcieAddr());

  std::string cmd            = odb_i->GetString(h_cmd,"Name");
  // std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
 
  // HNDLE h_cmd_par            = odb_i->GetHandle(0,parameter_path);
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);
    
  trkdaq::ControlRoc_DigiRW_Input_t  par;
  trkdaq::ControlRoc_DigiRW_Output_t pout;
    
  int  link        = odb_i->GetInteger(h_cmd,"link");

  par.rw           = odb_i->GetUInt16 (h_cmd_par,"rw");         //
  par.hvcal        = odb_i->GetUInt16 (h_cmd_par,"hvcal");      //
  par.address      = odb_i->GetUInt16 (h_cmd_par,"address");    //
  par.data[0]      = odb_i->GetUInt16 (h_cmd_par,"data[0]"); //
  par.data[1]      = odb_i->GetUInt16 (h_cmd_par,"data[1]"); //
  
  int  print_level = odb_i->GetInteger(h_cmd_par,"print_level");
   
  printf("dtc_i->fLinkMask: 0x%04x\n",_dtc_i->fLinkMask);
  _dtc_i->ControlRoc_DigiRW(&par,&pout,link,print_level,Stream);
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::DumpSettings(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"find_alignment");
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  // midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/dump_settings");
    
  int link        = odb_i->GetInteger(h_cmd,"link"       );    //

  int channel     = odb_i->GetInteger(h_cmd_par,"channel"    );    //
  int print_level = odb_i->GetInteger(h_cmd_par,"print_level");

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
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"get_key");
  //  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link               = odb_i->GetInteger(h_cmd,"link");

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
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"get_roc_design_info");
  // HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link               = odb_i->GetInteger(h_cmd,"link");

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
// 'roc_readout_mode' should be taken from the DAQ readout configuration
// 'emulate_cfo'      - from the DTC configuration
//-----------------------------------------------------------------------------
int TEqTrkDtc::InitReadout(std::ostream& Stream) {
  int rc(0);

  TLOG(TLVL_DEBUG) << "-- START";
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_dtc     = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_daq     = odb_i->GetDaqConfigHandle();
  
  uint32_t roc_readout_mode = odb_i->GetInteger(h_daq,"RocReadoutMode");
  uint32_t roc_lane_mask    = odb_i->GetUInt32(h_dtc,"RocLaneMask");

  TLOG(TLVL_DEBUG) << "roc_readout_mode:" << roc_readout_mode
                   << " roc_lane_mask:0x" << std::hex << roc_lane_mask;

  Stream << std::endl;
  try {
    _dtc_i->SetRocLaneMask(roc_lane_mask);
    rc = _dtc_i->InitReadout(-1,roc_readout_mode,&Stream);

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
// in the end, send ss.str() as a message to some log
//-----------------------------------------------------------------------------
int TEqTrkDtc::FindAlignment(HNDLE H_Cmd) { // std::ostream& Stream) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << std::format("-- START: _host_label:{} DTC{}",_host_label,_dtc_i->PcieAddr());

  std::stringstream sstr;
  StartMessage(H_Cmd,sstr);

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(H_Cmd);

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr()); // Handle(0,dtc_path);
  odb_i->SetStatus(h_dtc,1);

  int link        = odb_i->GetInteger(H_Cmd    ,"link"       );
  int print_level = odb_i->GetInteger(h_cmd_par,"print_level");
  int doit        = odb_i->GetInteger(h_cmd_par,"doit");

  TLOG(TLVL_DEBUG) << std::format("link:{} print_level:{} doit:{}",link,print_level,doit);

  sstr << std::endl;

  if (doit != 0) {
    try {
      rc = _dtc_i->FindAlignments(print_level,link,sstr);
    }
    catch (...) {
      sstr << " -- ERROR : coudn't execute FindAlignments for link:" << link << " ... BAIL OUT" << std::endl;
      rc = -10;
    }
  }
  else {
    TLOG(TLVL_DEBUG) << std::format("just waiting");
    ss_sleep(1000);
  }

  sstr << std::format(" rc:{}",rc);
  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str());

  odb_i->SetStatus(h_dtc,0);

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
// always do both CAL and HV, channel=-1: all 96 channels
//-----------------------------------------------------------------------------
int TEqTrkDtc::FindThresholds(std::ostream& Stream ) {

  TLOG(TLVL_DEBUG) << "-- START: not implemented yet";

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE h_cmd          = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string cmd_name = odb_i->GetString(h_cmd,"Name");
  // HNDLE h_cmd_par      = odb_i->GetHandle(h_cmd,cmd_name);
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  float threshold_mv = odb_i->GetFloat  (h_cmd_par,"threshold_mv");
  float tolerance_mv = odb_i->GetFloat  (h_cmd_par,"tolerance_mv");
  int   print_level  = odb_i->GetInteger(h_cmd_par,"print_level" );
  int   channel      = odb_i->GetInteger(h_cmd_par,"channel"     );
  
  int   ch1 = channel;
  int   ch2 = ch1+1;
  if (channel == -1) {
    ch1 =  0;
    ch2 = 96;
  }

  int link        = odb_i->GetInteger(h_cmd,"link"       );
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 link:" << link << " PcieAddr:" << _dtc_i->PcieAddr();

  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << "-- check 1";
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
// in ODB, set DTC status as busy
//-----------------------------------------------------------------------------
  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      _host_label.data(),_dtc_i->fPcieAddr);

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr());
  odb_i->SetStatus(h_dtc,1);

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path;

  uint16_t thr[6][96][2];
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
//-----------------------------------------------------------------------------
// TODO: make it a thread
//-----------------------------------------------------------------------------
    if (print_level > 0) {
      Stream << " -- link:" << lnk;
    }
    
    std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    HNDLE        h_panel    = odb_i->GetHandle(0,panel_path);
    
    TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path << " h_panel:" << h_panel;

    // midas::odb o(panel_path);

    // if (print_level > 1) {
    //   Stream << std::endl;
    //   Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
    //   Stream << "---------------------------------" << std::endl;
    // }

//     uint16_t ch_mask[96], gain_cal[96], gain_hv[96], thr_cal[96], thr_hv[96], data[4*96];
  
//     odb_i->GetArray(h_panel,"ch_mask" ,TID_WORD,ch_mask ,96);
//     odb_i->GetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
//     odb_i->GetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
//     odb_i->GetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
//     odb_i->GetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);

    for (int i=ch1; i<ch2; i++) {
//       data[i     ] = gain_cal[i];
//       data[i+96*1] = gain_hv [i];
//       data[i+96*2] = thr_cal [i];
//       data[i+96*3] = thr_hv  [i];

//       if (print_level > 1) {
//         Stream << std::format("{:3d} {:3d} {:5d} {:5d} {:5d} {:5d}\n",
//                               i,ch_mask[i],data[i],data[i+96*1],data[i+96*2],data[i+96*3]);
//       }
//     }

      // if (doit != 0) {
      _dtc_i->FindThreshold(lnk,i,0,threshold_mv,tolerance_mv,thr[lnk][i][0]);
      _dtc_i->FindThreshold(lnk,i,1,threshold_mv,tolerance_mv,thr[lnk][i][1]);
      // }

      Stream << "done with ch:" << i << "trh[0]:" << thr[lnk][i][0] << "thr[1]:" << thr[lnk][i][1] << std::endl;
    }
  }
  
  // the missing part is to store the thresholds ... later
    
  odb_i->SetStatus(h_dtc,0);
  
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::LoadThresholds(HNDLE h_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream sstr;

  StartMessage(h_Cmd,sstr);
  
  OdbInterface* odb_i  = OdbInterface::Instance();
  
  int link        = odb_i->GetInteger(h_Cmd    ,"link"       );
  
  HNDLE h_cmd_par = odb_i->GetCmdParameterHandle(h_Cmd);
  int doit        = odb_i->GetInteger(h_cmd_par,"doit"       );
  int print_level = odb_i->GetInteger(h_cmd_par,"print_level");

  
  
  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << std::format(" checkpoint 0.1 PcieAddr:{} lnk1:{} lnk2:{} print_level:0x{:04x}",
                                  _dtc_i->PcieAddr(),lnk1,lnk2,print_level);
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
// in ODB, set DTC status as busy
// get config directory on disk
//-----------------------------------------------------------------------------
  std::string config_dir = odb_i->GetConfigDir();
    
  HNDLE h_tracker = odb_i->GetHandle(0,"/Mu2e/ActiveRunConfiguration/Tracker");
  std::string thresholds_dir = odb_i->GetString(h_tracker,"ReadoutConfiguration/thresholds_dir");

  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      _host_label.data(),_dtc_i->PcieAddr());

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr()); // Handle(0,dtc_path);
  odb_i->SetStatus(h_dtc,1);

  TLOG(TLVL_DEBUG) << std::format(" check 1.1 dtc_path:{} thresholds_dir:{}",dtc_path,thresholds_dir);
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
    int link_rc(0);

    sstr << std::format("-- load thresholds: DTC{}:link{}",_dtc_i->PcieAddr(),lnk);
    std::string  panel_path = std::format("{:s}/Link{}/DetectorElement",dtc_path.data(),lnk);
    
    TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path;

    HNDLE h_panel          = odb_i->GetHandle(0,panel_path);
    std::string panel_name = odb_i->GetString(h_panel,"Name");
    int mnid               = atoi(panel_name.substr(2).data());
    int sdir               = (mnid/10)*10;

    TLOG(TLVL_DEBUG) << std::format("check 1.3 panel_name:{} mnid:{:03d} sdir:{}",panel_name,mnid,sdir);

    try {
      link_rc = 1;

      int slot_id  = odb_i->GetInteger(h_panel,"slot_id");    // 20*slot+panel_slot
      int slot     = (slot_id/20);
      TLOG(TLVL_DEBUG) << std::format("-- check 1.4 slot:{}",slot);
      link_rc = 2;
      
      std::string fn = std::format("{:}/tracker/slot_{:02d}/{:s}/{:s}.json",
                                   config_dir.data(),slot,thresholds_dir.data(),panel_name.data());
    
      TLOG(TLVL_DEBUG) << "-- check 1.5 fn:" << fn;
      link_rc = 3;

      std::ifstream ifs(fn);
      
      if (not ifs.is_open()) {
        std::string msg = std::format("failed to open file:{}\n",fn);
        TLOG(TLVL_ERROR) << msg;
        sstr << " ERROR: " << msg; 
        rc -= 1;
        continue;
      }
      
      nlohmann::json jf = nlohmann::json::parse(ifs);
    
      if (print_level > 1) {
        sstr << std::endl;
        sstr << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
        sstr << "---------------------------------" << std::endl;
      }
    
      link_rc = 4;

      uint16_t thr_cal[96], thr_hv[96], gain_cal[96], gain_hv[96];

      odb_i->GetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
      odb_i->GetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);
      odb_i->GetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
      odb_i->GetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
      
      link_rc = 5;

      for (auto& elm : jf.items()) {
        nlohmann::json val = elm.value();
        uint16_t ich       = val["channel"  ];
        uint16_t gain      = val["gain"     ];
        uint16_t thr       = val["threshold"];
        std::string type   = val["type"     ];
        
        TLOG(TLVL_DEBUG) << "-- check 1.6 jf.ich:" << ich << " type:" << type
                         << " gain:" << gain << " thr:" << thr;
        
        if (print_level > 1) {
          sstr << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;
          TLOG(TLVL_DEBUG+1) << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;
        }
        
        if (doit != 0) {
          std::string thr_key, gain_key;
          if      (type == "cal") {
            thr_cal [ich] = thr;
            gain_cal[ich] = gain;
          }
          else if (type == "hv" ) {
            thr_hv [ich] = thr;
            gain_hv[ich] = gain;
          }
        }
      }
      
      link_rc = 6;
      odb_i->SetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
      odb_i->SetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);
      odb_i->SetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
      odb_i->SetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
      
      sstr << std::format(" panel_name:{} slot:{} : SUCCESS\n",panel_name,slot);
    }
    catch (...) {
      sstr << std::format(" ERROR, panel_name:{} link_rc:{}\n",panel_name,link_rc);
      TLOG(TLVL_ERROR) << "link:" << lnk << " panel_name:" << panel_name << " link_rc:" << link_rc;
      rc -= 1;
    }
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order 
//-----------------------------------------------------------------------------
  odb_i->SetStatus(h_dtc,rc);

  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str());
  
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);

  return rc;
}

//-----------------------------------------------------------------------------
// read all thresholds
//-----------------------------------------------------------------------------
int TEqTrkDtc::MeasureThresholds(std::ostream& Stream) {

  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i  = OdbInterface::Instance();

  HNDLE h_cmd          = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string cmd_name = odb_i->GetString(h_cmd,"Name");
  HNDLE h_cmd_par      = odb_i->GetHandle(h_cmd,cmd_name);
  // HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int   link           = odb_i->GetInteger(h_cmd    ,"link"       );
  int   print_level    = odb_i->GetInteger(h_cmd_par,"print_level");
  
  TLOG(TLVL_DEBUG) << "- checkpoint 0.1 Link:" << link
                   << " PcieAddr:" << _dtc_i->PcieAddr()
                   << " PrintLevel:" << print_level;

  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << "-------------- lnk2, lnk2:" << lnk1 << " " << lnk2;

  std::vector<float> thr;

  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) {
      Stream << "Link:" << lnk << " is disabled" << std::endl;
      continue;
    }
    
    TLOG(TLVL_DEBUG) << " -- link:" << lnk << " enabled";
//-----------------------------------------------------------------------------
// exceptions are handled in the called function
//-----------------------------------------------------------------------------
    rc = _dtc_i->ControlRoc_ReadThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                           print_level,Stream);
    if (rc < 0) break;
//-----------------------------------------------------------------------------
// now store the output in the ODB, the packing order: (hv, cal) ... ignore 'tot')
//-----------------------------------------------------------------------------
    std::string p_panel = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                      _host_label.data(),_dtc_i->PcieAddr(),lnk);
    midas::odb o_panel(p_panel);
    for (int i=0; i<96; ++i) {
      o_panel["thr_hv_mv" ][i] = thr[3*i  ];
      o_panel["thr_cal_mv"][i] = thr[3*i+1];
    }

    _dtc_i->ControlRoc_PrintThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                       print_level,Stream);
  }

  TLOG(TLVL_DEBUG) << "-- END, rc:" << rc;
  return rc;
}

//-----------------------------------------------------------------------------
// link=-1: print status of all enabled ROCs
//-----------------------------------------------------------------------------
int TEqTrkDtc::PrintRocStatus(std::ostream& Stream) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());

  int link        = odb_i->GetInteger(h_cmd,"link");
  
  TLOG(TLVL_DEBUG) << std::format("link:{}",link);
  try         {
    _dtc_i->PrintRocStatus(1,link,Stream);
  }
  catch (...) {
    Stream << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl;
  }
  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ProgramRoc(std::ostream& Stream) {
  TLOG(TLVL_DEBUG) << "-- START TEqTrkDtc::" << __func__;
  
  // OdbInterface* odb_i     = OdbInterface::Instance();
  // HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  // HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  // int link        = odb_i->GetInteger(h_cmd,"link"       );
  // int print_level = odb_i->GetInteger(h_cmd_par,"print_level");

  // if (link == -1) Stream << std::endl;

  // try {
  //   _dtc_i->FindAlignments(print_level,link,Stream);
  // }
  // catch (...) {
  //   Stream << " -- ERROR : coudn't execute FindAlignments for link:" << link << " ... BAIL OUT" << std::endl;
  // }
  
  TLOG(TLVL_DEBUG) << "-- END TEqTrkDtc::" << __func__;
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::PulserOff(std::ostream& Stream) {
  int rc(0);

  TLOG(TLVL_DEBUG) << "-- START";
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link, print_level;

  link               = odb_i->GetInteger(h_cmd    ,"link"              );    //
  print_level        = odb_i->GetInteger(h_cmd_par,"print_level"       );

  TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOff, link:" << link;
  try {
    _dtc_i->ControlRoc_PulserOff(link,print_level,Stream);
  }
  catch(...) {
    Stream << "ERROR : coudn't execute ControlRoc_PulserOFF ... BAIL OUT" << std::endl;
  }
  
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::PulserOn(std::ostream& Stream) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link, first_channel_mask, duty_cycle, pulser_delay, print_level;

  link               = odb_i->GetInteger(h_cmd    ,"link"              );    //
  
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
  
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::Rates(std::ostream& Stream) {
  // int timeout_ms(150);

  std::vector<uint16_t> rates  [6];   // 6 ROCs max
  std::vector<int>      ch_mask[6];   // used for printing only

  uint16_t              rates_ch_mask[6]; // cached masks from the RATES command ODB record

  TLOG(TLVL_DEBUG) << "-- START";
  
  OdbInterface* odb_i      = OdbInterface::Instance();
  HNDLE         h_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   conf_name  = odb_i->GetRunConfigName(h_run_conf);
  
  trkdaq::ControlRoc_Rates_t  prates;
    
  HNDLE h_cmd          = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string cmd_name = odb_i->GetString(h_cmd,"Name");
  HNDLE h_cmd_par      = odb_i->GetHandle(h_cmd,cmd_name);

  prates.num_lookback         = odb_i->GetInteger(h_cmd_par,"num_lookback");    //
  prates.num_samples          = odb_i->GetInteger(h_cmd_par,"num_samples" );    //
  int  print_level            = odb_i->GetInteger(h_cmd_par,"print_level" );
  int  use_panel_channel_mask = odb_i->GetInteger(h_cmd_par,"use_panel_channel_mask");

  int  link                   = odb_i->GetInteger(h_cmd    ,"link");

  for (int i=0; i<6; i++) {
    rates  [i].reserve(96);
    ch_mask[i].reserve(96);
  }

  int lnk1(link), lnk2(link+1);
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue ;
    
    if (use_panel_channel_mask == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
// '6' below is a random coincidence
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; ++iw) {
        char key[16];
        sprintf(key,"ch_mask[%i]",iw);
        uint16_t w = odb_i->GetUInt16(h_cmd_par,key);
        // prates.ch_mask[iw] = w;
        rates_ch_mask [iw] = w;         // cache it here 

        for (int k=0; k<16; ++k) {
          // int loc = iw*16+k;
          int flag = (w >> k) & 0x1;
          // Stream << "mask loc:" << loc << " flag:" << flag <<std::endl;
          ch_mask[lnk].push_back(flag);
        }
      }
    }
    else {
//-----------------------------------------------------------------------------
// use straw mask defined by the panel
//-----------------------------------------------------------------------------
      std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                            conf_name,_host_label.data(),_dtc_i->PcieAddr(),lnk);
      midas::odb   odb_panel(panel_path);
      std::string  panel_name = odb_panel["Name"];

      if (print_level&0x8) Stream << "panel_path:" << panel_path << " panel_name:" << panel_name << std::endl;

      for (int i=0; i<96; ++i) {
        uint16_t on_off = odb_panel["ch_mask"][i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          rates_ch_mask[iw] = 0;
        }
        rates_ch_mask[iw] |= (on_off << ib);
        // Stream << "ch_mask["<<i<<"]:" << on_off << " iw:" << iw << " ib:" << ib << std::endl;

        ch_mask[lnk].push_back(on_off);
      }
    }
//-----------------------------------------------------------------------------
// print the mask
//-----------------------------------------------------------------------------
    if (print_level&0x8) {
      Stream << " par.ch_mask:";
      for (int iw=0; iw<6; iw++) {
        Stream << " " << std::hex << rates_ch_mask[iw];
      }
      Stream << std::endl;
    }

    try {
//-----------------------------------------------------------------------------
// switch to internal clock to measure the rates
// assume that, with the exception of the clock_mode, which we flip, the ODB has the right set
// of parameters stored
//-----------------------------------------------------------------------------
      if (print_level & 0x8) {
        Stream <<  std::format("dtc_i->fLinkMask: 0x{:06x}",_dtc_i->fLinkMask) << std::endl;
      }

      // midas::odb o_read_cmd   ("/Mu2e/Commands/Tracker/DTC/control_roc_read");
      HNDLE h_read_par      = odb_i->GetHandle(h_cmd,"read");

      trkdaq::ControlRoc_Read_Input_t0 pread;

      pread.adc_mode        = odb_i->GetUInt16(h_read_par,"adc_mode"     );   // -a
      pread.tdc_mode        = odb_i->GetUInt16(h_read_par,"tdc_mode"     );   // -t 
      pread.num_lookback    = odb_i->GetUInt16(h_read_par,"num_lookback" );   // -l 
  
      pread.num_samples     = odb_i->GetUInt16(h_read_par,"num_samples"  );   // -s
      pread.num_triggers[0] = odb_i->GetUInt16(h_read_par,"num_triggers[0]"); // -T 10
      pread.num_triggers[1] = odb_i->GetUInt16(h_read_par,"num_triggers[1]"); //
//-----------------------------------------------------------------------------
// when reading RATES, always read all channels, no matter what the current settings are
// and the reasonable settings could be 1) ALL CHANNELS 2) a channel mask is defined by the panel
// rely on the RATES command to have that setting right - defined by the panel
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; iw++) pread.ch_mask[iw] = 0XFFFF; // prates.ch_mask[iw];
//-----------------------------------------------------------------------------
// this is a tricky place: rely on that the READ command ODB record
// stores the -p value used during the data taking
//-----------------------------------------------------------------------------
      pread.enable_pulser   = odb_i->GetUInt16(h_read_par,"enable_pulser");   // -p 1
      pread.marker_clock    = 0;                             // to read the rates, enable internal clock
      pread.mode            = odb_i->GetUInt16(h_read_par,"mode"         );   // 
      pread.clock           = odb_i->GetUInt16(h_read_par,"clock"        );   //

      if (print_level & 0x8) {
        Stream <<  "--- running control_roc_read marker_clock:" << pread.marker_clock
               << " enable_pulser:" << pread.enable_pulser << std::endl;
      }

      _dtc_i->ControlRoc_Read(&pread,lnk,print_level,Stream);
      
      if (print_level & 0x8) Stream <<  "--- running control_roc_rates" << std::endl;

      for (int iw=0; iw<6; iw++) prates.ch_mask[iw] = 0XFFFF; // want to read all channels, RATES doesn't change any masks

      _dtc_i->ControlRoc_Rates(lnk,&rates[lnk],print_level,&prates,&Stream);

      pread.marker_clock    = odb_i->GetUInt16(h_read_par,"marker_clock" );   // recover marker_clock mode

      if (print_level & 0x8) {
        Stream <<  "--- running control_roc_read marker_clock:" << pread.marker_clock
               << " enable_pulser:" << pread.enable_pulser << std::endl;
      }
//-----------------------------------------------------------------------------
// restore the channel mask relying on the RATES command parameters to define it right:
// either all channels are enabled, or the channel mask is defined by the panel
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; iw++) pread.ch_mask[iw] = rates_ch_mask[iw];
      _dtc_i->ControlRoc_Read(&pread,lnk,print_level,Stream);

      if (print_level & 0x2) { // detailed printout, one ROC only
        _dtc_i->PrintRatesSingleRoc(&rates[lnk],&ch_mask[lnk],Stream);
      }
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_rates for link:" << lnk << "... BAIL OUT";
      return -1;
    }
  }
//-----------------------------------------------------------------------------
// if we got here, the execution succeeded, print
//-----------------------------------------------------------------------------
  if (print_level & 0x4) {
    _dtc_i->PrintRatesAllRocs(rates,ch_mask,Stream);
  }
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::Read(std::ostream& Stream) {
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i      = OdbInterface::Instance();
  HNDLE         h_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   conf_name  = odb_i->GetRunConfigName(h_run_conf);

  TLOG(TLVL_DEBUG) << "conf_name:" << conf_name;
  
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  std::string   cmd_name  = odb_i->GetString(h_cmd,"Name");
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  trkdaq::ControlRoc_Read_Input_t0 par;

  par.adc_mode        = odb_i->GetUInt16(h_cmd_par,"adc_mode"     );   // -a
  par.tdc_mode        = odb_i->GetUInt16(h_cmd_par,"tdc_mode"     );   // -t 
  par.num_lookback    = odb_i->GetUInt16(h_cmd_par,"num_lookback" );   // -l 
  
  par.num_samples     = odb_i->GetUInt16(h_cmd_par,"num_samples"    ); // -s
  par.num_triggers[0] = odb_i->GetUInt16(h_cmd_par,"num_triggers[0]"); // -T 10
  par.num_triggers[1] = odb_i->GetUInt16(h_cmd_par,"num_triggers[1]"); //

  // this is how we get the panel name

  int use_panel_channel_mask = odb_i->GetInteger(h_cmd_par,"use_panel_channel_mask");
  int print_level            = odb_i->GetInteger(h_cmd_par,"print_level");
    
  // Stream << "use_panel_channel_mask:" <<  use_panel_channel_mask << std::endl;

  int lnk1 = odb_i->GetInteger(h_cmd,"link");  // note where it is coming from
  
  int lnk2 = lnk1+1;
  if (lnk1 == -1) {
    Stream << std::endl;
    lnk1 = 0;
    lnk2 = 6;
  }

  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    Stream << "-- link:" << lnk;
    if (_dtc_i->LinkEnabled(lnk) == 0) {
      if (print_level != 0) {
        Stream << " : disabled" << std::endl;
        continue;
      }
    }
    TLOG(TLVL_DEBUG) << "use_panel_channel_mask:" << use_panel_channel_mask;
    if (use_panel_channel_mask == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
//-----------------------------------------------------------------------------
      odb_i->GetArray(h_cmd_par,"ch_mask",TID_WORD,par.ch_mask,6);
    }
    else {
//-----------------------------------------------------------------------------
// use straw mask defined for the panels, save the masks in the command ODB record
// the missing part - need to know the node name. But that is the local host name,
// the one the frontend is running on
// _read mask: 6 ushort's
//-----------------------------------------------------------------------------
      std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                            conf_name.data(),_host_label.data(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_DEBUG) << "panel_path:" << panel_path;
      midas::odb   odb_panel(panel_path);
      for (int i=0; i<96; ++i) {
        int on_off = odb_panel["ch_mask"][i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          par.ch_mask[iw] = 0;
        }
        // Stream << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
        par.ch_mask[iw] |= on_off << ib;
      }

      if (print_level & 0x4) {
        Stream << " par.ch_mask:";
        for (int i=0; i<6; i++) {
          Stream << " " << std::hex << par.ch_mask[i];
        }
        // Stream << std::endl;
      }
    }
        
    par.enable_pulser   = odb_i->GetUInt16(h_cmd_par,"enable_pulser");   // -p 1
    par.marker_clock    = odb_i->GetUInt16(h_cmd_par,"marker_clock" );   // -m 3: data taking (ext clock), -m 0: rates (int clock)
    par.mode            = odb_i->GetUInt16(h_cmd_par,"mode"         );   // not used ?
    par.clock           = odb_i->GetUInt16(h_cmd_par,"clock"        );   // 
  
    try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
      _dtc_i->ControlRoc_Read(&par,lnk,print_level,Stream);
      Stream << " SUCCESS" << std::endl;
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_Read for link:" << lnk << " ... BAIL OUT" << std::endl;
    }
  }
  
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");
  //   HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

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
int TEqTrkDtc::ReadRocRegister(std::ostream& Stream) {
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_roc_register");
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
int TEqTrkDtc::ReadMnID(std::ostream& Stream) {
  int rc(0);

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(HostLabel(),_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link         = odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];
  int print_level  = odb_i->GetInteger(h_cmd_par,"print_level"); // o["print_level"];

  TLOG(TLVL_DEBUG) << std::format("-- START: DTC:{} link:{} print_level:{}",_dtc_i->PcieAddr(),link,print_level);

  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  int pcie_addr = _dtc_i->PcieAddr();
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
    if (not _dtc_i->LinkEnabled(lnk)) {
      std::string msg = std::format("DTC:{} link:{} not enabled",pcie_addr,lnk);
      TLOG(TLVL_ERROR) << msg;
      Stream << "ERROR: " << msg << "\n";
      rc -= 1;
      continue;
    }

    if (not _dtc_i->LinkLocked(lnk)) {
      std::string msg = std::format("DTC:{} link:{} enabled but not locked",pcie_addr,lnk);
      TLOG(TLVL_ERROR) << msg;
      Stream << "ERROR: " << msg << "\n";
      rc -= 10;
      continue;
    }

    int mnid = _dtc_i->ReadPanelID(lnk,print_level);
    Stream << std::format("DTC:{} link:{} mnid:{}\n",pcie_addr,lnk,mnid);
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadIlp(std::ostream& Stream) {
  int rc(0);

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string   cmd_name  = odb_i->GetString(h_cmd,"Name");
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link        = odb_i->GetInteger(h_cmd    ,"link"    );

  int print_level  = odb_i->GetInteger(h_cmd_par,"print_level");
  
  if (not _dtc_i->LinkEnabled(link)) {
    std::string msg = std::format("DTC:{} link:{} not enabled",_dtc_i->PcieAddr(),link);
    TLOG(TLVL_ERROR) << msg;
    Stream << "ERROR: " << msg << "\n";
    return -1;
  }

  if (not _dtc_i->LinkLocked(link)) {
    std::string msg = std::format("DTC:{} link:{} enabled but not lockd",_dtc_i->PcieAddr(),link);
    TLOG(TLVL_ERROR) << msg;
    Stream << "ERROR: " << msg << "\n";
    return -2;
  }
  
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

  TLOG(TLVL_DEBUG) << std::format("--START:");

                                  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(HostLabel(),_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link         = odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];
  int print_level  = odb_i->GetInteger(h_cmd_par,"print_level"); // o["print_level"];

  _odb_i->SetStatus(_handle,1);
  
  try         {
    if (link != -1) {
      std::vector<uint16_t>   data;
      _dtc_i->ControlRoc_ReadSpi(data,link,print_level,Stream);
    }
    else {
                                        // need formatted printout for all ROCs
      trkdaq::TrkSpiData_t spi[6];
      for (int i=0; i<6; i++) {
                                        // to print
        link = i;
        if (_dtc_i->LinkEnabled(i)) {
          _dtc_i->ControlRoc_ReadSpi_1(&spi[i],i,0,Stream);
        }
      }
                                        // now - printing
      _dtc_i->PrintSpiAll(spi,Stream);
    }

    _odb_i->SetStatus(_handle,1);
  }
  catch (...) {
//-----------------------------------------------------------------------------
// send an error message and print 
//-----------------------------------------------------------------------------
    std::string msg = std::format("TEqTrkDtc::{}: ERROR reading ROC SPI dtc:{} link:{}",
                                  __func__,_dtc_i->PcieAddr(),link);
    cm_msg(MERROR, HostLabel().data(),msg.data());    
    Stream << msg << " on " << HostLabel() << std::endl;
    rc = -1;
  }

  TLOG(TLVL_DEBUG) << std::format("--END: rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadDDR(std::ostream& Stream) {
  int rc(0);
  // midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_ddr");

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  // std::string   cmd_name  = odb_i->GetString(h_cmd,"Name");
  // HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,cmd_name);
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int link         = odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];
  int block_number = odb_i->GetInteger(h_cmd_par,"block_number");
  // int print_level  = odb_i->GetInteger(h_cmd_par,"print_level"); // o["print_level"];

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
int TEqTrkDtc::RebootMcu(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/reboot_mcu");

  HNDLE         h_cmd     = _odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  std::string   cmd_name  = _odb_i->GetString(h_cmd,"Name");

  int link         = _odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];

  //  int print_level  = o["print_level"];
  
  rc = _dtc_i->RebootMcu(link);

  if (rc == 0) Stream << " -- reboot_mcu OK";
  else         Stream << " -- ERROR: failed reboot_mcu link:" << link << " rc:" << rc << std::endl;
  
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::ResetRoc(std::ostream& Stream) {
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
int TEqTrkDtc::SetCalDac(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/set_caldac");
    
  HNDLE        h_cmd     = _odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = _odb_i->GetCmdParameterHandle(h_cmd);

  int link               = _odb_i->GetInteger(h_cmd    ,"link"       ); // o["link"       ];

  int first_channel_mask = _odb_i->GetInteger(h_cmd_par,"first_channel_mask");    //
  int value              = _odb_i->GetInteger(h_cmd_par,"value"             );    //
  int print_level        = _odb_i->GetInteger(h_cmd_par,"print_level"       );

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
int TEqTrkDtc::SetThresholds(HNDLE H_Cmd) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream sstr;

  StartMessage(H_Cmd,sstr);

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(H_Cmd);
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  // int doit        = o["doit"] ;
  // int print_level = o["print_level"] ;

  int link        = odb_i->GetInteger(H_Cmd,"link"       );

  int doit        = odb_i->GetInteger(h_cmd_par,"doit"       );
  int print_level = odb_i->GetInteger(h_cmd_par,"print_level");
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 link:" << link << " PcieAddr:" << _dtc_i->PcieAddr();

  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << "-- check 1";
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
// in ODB, set DTC status as busy
//-----------------------------------------------------------------------------
  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      _host_label.data(),_dtc_i->fPcieAddr);

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr());
  odb_i->SetStatus(h_dtc,1);

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path;
  for (int lnk=lnk1; lnk<lnk2; lnk++) {

    if (print_level > 0) {
      sstr << " -- link:" << lnk;
    }

    if (not _dtc_i->LinkEnabled(lnk)) {
      sstr << std::format(" : disabled\n");
      continue;
    }
    else if (not _dtc_i->LinkLocked(lnk)) {
      sstr << " ERROR : link enabled but not locked\n";
      TLOG(TLVL_ERROR) << std::format("host:{} DTC:{} link:{} enabled but not locked",_host_label,_dtc_i->PcieAddr(),lnk);
    }
    
    std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    HNDLE        h_panel    = odb_i->GetHandle(0,panel_path);
    
    TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path << " h_panel:" << h_panel;

    // midas::odb o(panel_path);

    if (print_level > 1) {
      sstr << std::endl;
      sstr << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
      sstr << "---------------------------------" << std::endl;
    }

    uint16_t ch_mask[96], gain_cal[96], gain_hv[96], thr_cal[96], thr_hv[96], data[4*96];
  
    odb_i->GetArray(h_panel,"ch_mask" ,TID_WORD,ch_mask ,96);
    odb_i->GetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
    odb_i->GetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
    odb_i->GetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
    odb_i->GetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);

    for (int i=0; i<96; i++) {
      data[i     ] = gain_cal[i];
      data[i+96*1] = gain_hv [i];
      data[i+96*2] = thr_cal [i];
      data[i+96*3] = thr_hv  [i];

      if (print_level > 1) {
        sstr << std::format("{:3d} {:3d} {:5d} {:5d} {:5d} {:5d}\n",
                            i,ch_mask[i],data[i],data[i+96*1],data[i+96*2],data[i+96*3]);
      }
    }
      
    if (doit != 0) {
      try {
        _dtc_i->ControlRoc_SetThresholds(lnk,data);
        sstr << " : SUCCESS" ;
      }
      catch(...) {
        sstr << " : ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
      }
    }
    sstr << std::endl;
  }
  
  odb_i->SetStatus(h_dtc,0);
  
  int cmd_rc = TMu2eEqBase::WriteOutput(sstr.str());

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{} cmd_rc:{}",rc,cmd_rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::WriteRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");
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
int TEqTrkDtc::TestCommand(std::ostream& Stream) {
  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);  // "ParameterPath"

  try {
    Stream << "test_command finished OK" << std::endl;
  }
  catch (...) {
    TLOG(TLVL_ERROR) << std::format("rc:{}",rc);
    Stream << std::format(" ERROR : test_command rc:{}\n",rc);
  }

  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);
  return 0;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::WriteRocRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCmdHandle(_host_label,_dtc_i->PcieAddr());
  //  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"write_roc_register");
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


//-----------------------------------------------------------------------------
// for single-link commands aim for a 1 line output,
// for 'all-active-link' comamnds (link=-1) print the header and then - 
// one line per link
//-----------------------------------------------------------------------------
int TEqTrkDtc::StartMessage(HNDLE h_Cmd, std::ostream& Stream) {

  Stream << std::endl; // perhaps

  std::string cmd  = _odb_i->GetString (h_Cmd,"Name");
  int link         = _odb_i->GetInteger(h_Cmd,"link");
  
  Stream << std::format("-- label:{} host:{} cmd:{} pcie_addr:{} link:{}",
                        HostLabel(),FullHostName(),cmd,_dtc_i->PcieAddr(),link);
  Stream << std::endl;
  return 0;
}
