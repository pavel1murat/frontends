///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include "odbxx.h"

#include "utils/OdbInterface.hh"
#include "node_frontend/TEqTrkDtc.hh"
#include "utils/TEquipmentManager.hh"
#include "otsdaq-mu2e-tracker/Ui/ControlRocTypes.hh"
#include "otsdaq-mu2e-tracker/Ui/TrackerRegisters.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTrkDtc"
namespace {
//                                            Temp, VCCINT, VCCAUX, VCBRAM
  std::initializer_list<int>  DtcRegHist = { 0x9010, 0x9014, 0x9018, 0x901c };

  std::initializer_list<int>  DtcRegisters = {
    0x9004, // 0
    0x9100,
    0x9114,
    0x9140,
    0x9144,
    
    0x9158,                             // 5
    0x9188,
    0x91a8,
    0x91ac,
    0x91bc,
    
    0x91c0,                             // 10
    0x91c4,
    0x91f4,
    0x91f8,
    0x93e0,
    
    0x9650,                             // 15 TX HB packet count link 0
    0x9654,                             // TX HB packet count link 1
    0x9658,
    0x965c,
    0x9660,
    
    0x9664,
    0x9668                              // 20 TX HB packet count CFO (16 bits)
  };
  
  // some ROC registers are listed in decimal format, and some - in hex
  std::initializer_list<int> RocRegisters = {
    trkdaq::registers::rocdcs::DBG,
    trkdaq::registers::rocdcs::ROC_STATUS,
    trkdaq::registers::rocdcs::ROC_ENABLE,
    trkdaq::registers::rocdcs::DCS_DDR_ADDRESS_L,
    trkdaq::registers::rocdcs::DCS_DDR_ADDRESS_H,
    trkdaq::registers::rocdcs::LOSS_LOCK,
    trkdaq::registers::rocdcs::TWI_CONTROL,
    trkdaq::registers::rocdcs::LOOPBACK_COARSE_DELAY,
    trkdaq::registers::rocdcs::DREQ_FIFO_WRCNT,
    trkdaq::registers::rocdcs::DREQ_FIFO_WR_STATUS,
    trkdaq::registers::rocdcs::DREQ_FIFO_RDCNT,
    trkdaq::registers::rocdcs::DREQ_FIFO_RD_STATUS,
    trkdaq::registers::rocdcs::EWM_CNT_L,
    trkdaq::registers::rocdcs::EWM_CNT_H,
    trkdaq::registers::rocdcs::DCS_EVMCNT_L, // was 65
    trkdaq::registers::rocdcs::DCS_EVMCNT_H,
    trkdaq::registers::rocdcs::DCS_HB_CNT_L, // was 17
    trkdaq::registers::rocdcs::DCS_HB_CNT_H,
    trkdaq::registers::rocdcs::DCS_NULLHB_CNT_L,
    trkdaq::registers::rocdcs::DCS_NULLHB_CNT_H,
    trkdaq::registers::rocdcs::DCS_HBCNT_ONHOLD_L,
    trkdaq::registers::rocdcs::DCS_HBCNT_ONHOLD_H,
    trkdaq::registers::rocdcs::DCS_PREFCNT_L,
    trkdaq::registers::rocdcs::DCS_PREFCNT_H,
    trkdaq::registers::rocdcs::DATAREQ_CNT_L,
    trkdaq::registers::rocdcs::DATAREQ_CNT_H,
    trkdaq::registers::rocdcs::DCS_DREQCNT_L,
    trkdaq::registers::rocdcs::DCS_DREQCNT_H,
    trkdaq::registers::rocdcs::IS_SKIPPED_DREQ_CNT,
    trkdaq::registers::rocdcs::DCS_DREQREAD_L,
    trkdaq::registers::rocdcs::DCS_DREQREAD_H,
    trkdaq::registers::rocdcs::DCS_DREQSENT_L, // was 38
    trkdaq::registers::rocdcs::DCS_DREQSENT_H,
    trkdaq::registers::rocdcs::DCS_DREQNULL_L,
    trkdaq::registers::rocdcs::DCS_DREQNULL_H,
    trkdaq::registers::rocdcs::DCS_SPILLCNT_L,
    trkdaq::registers::rocdcs::DCS_SPILLCNT_H,
    trkdaq::registers::rocdcs::DCS_HBTAG_0,
    trkdaq::registers::rocdcs::DCS_HBTAG_1,
    trkdaq::registers::rocdcs::DCS_PREFTAG_0,
    trkdaq::registers::rocdcs::DCS_PREFTAG_1,
    trkdaq::registers::rocdcs::DCS_FETCHTAG_0,
    trkdaq::registers::rocdcs::DCS_FETCHTAG_1,
    trkdaq::registers::rocdcs::DCS_DREQTAG_0,
    trkdaq::registers::rocdcs::DCS_DREQTAG_1,
    trkdaq::registers::rocdcs::DCS_OFFSETTAG_0,
    trkdaq::registers::rocdcs::DCS_OFFSETTAG_1,
    trkdaq::registers::rocdcs::HB_TAG_ERR_CNT,
    trkdaq::registers::rocdcs::HB_DREQ_ERR_CNT,
    trkdaq::registers::rocdcs::HB_LOST_CNT,
    trkdaq::registers::rocdcs::EWM_LOST_CNT,
    trkdaq::registers::rocdcs::DTC_PKT_COUNT,
    trkdaq::registers::rocdcs::DCS_PKT_COUNT,
    trkdaq::registers::rocdcs::DREQ_PKT_COUNT,
    trkdaq::registers::rocdcs::DREQ_HDR_PKT_COUNT,
    trkdaq::registers::rocdcs::DREQ_DATA_PKT_COUNT,
    trkdaq::registers::rocdcs::DREQ_EMPTY_PKT_COUNT
  };
  
};

//-----------------------------------------------------------------------------
TEqTrkDtc::TEqTrkDtc(const char* Name, const char* Title) : TMu2eEqBase(Name,Title,TMu2eEqBase::kTracker) {
}

//-----------------------------------------------------------------------------
TEqTrkDtc::~TEqTrkDtc() {
}

//-----------------------------------------------------------------------------
TEqTrkDtc::TEqTrkDtc(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Dtc): TMu2eEqBase(Name,Title,TMu2eEqBase::kTracker)
{
  TLOG(TLVL_DEBUG) << "-- START: H_RunConf:" << H_RunConf << " H_Dtc:" << H_Dtc;
  
  _handle              = H_Dtc;
  //  HNDLE h_daq          = _odb_i->GetHandle(H_RunConf,"DAQ");

  int dtc_enabled      = _odb_i->GetEnabled       (H_Dtc);
  int pcie_addr        = _odb_i->GetDtcPcieAddress(H_Dtc);
  int link_mask        = _odb_i->GetLinkMask      (H_Dtc);

  _cmd_handle          = _odb_i->GetDtcCmdHandle(_host_label,pcie_addr);

  KEY key;
  _odb_i->GetKey(H_Dtc,&key);
  TLOG(TLVL_DEBUG) << "key.name:" << key.name;
  SetName(key.name);
//-----------------------------------------------------------------------------
// for now, disable the DTC re-initialization
//-----------------------------------------------------------------------------
  int skip_dtc_init    = _odb_i->GetSkipDtcInit   (H_RunConf);
  
  TLOG(TLVL_DEBUG) << "link_mask:0x" << std::hex << link_mask << " pcie_addr:" << pcie_addr;
  
  std::string subsystem = _odb_i->GetString(H_Dtc,"Subsystem");
  if (subsystem == "tracker") {
    _dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
    _dtc_i->fSubsystem = mu2edaq::kTracker;
  }
  else {
    return;
  }
  
//-----------------------------------------------------------------------------
// start from checking the DTC FW verion and comparing it to the required one -
// defined in ODB
//-----------------------------------------------------------------------------
  uint32_t required_fw_version = _odb_i->GetRequiredFwVersion(H_Dtc);
  uint32_t fw_version          = _dtc_i->ReadRegister(0x9004);

  if ((required_fw_version != 0) and (fw_version != required_fw_version)) {
    std::string msg = std::format("DTC{}@{} has fw version:0x{:08x} different from required version:0x{:08x}",
                                  _dtc_i->PcieAddr(),HostLabel(),fw_version,required_fw_version);
    TLOG(TLVL_ERROR) << msg;
                                       // and send an error message
    cm_msg(MERROR, __func__,msg.data());
    cm_msg_flush_buffer();
    SetStatus(-1);
  }
  else {
    _dtc_i->fPcieAddr       = pcie_addr;
    _dtc_i->fEnabled        = dtc_enabled;

    _dtc_i->fDtcID          = _odb_i->GetDtcID         (H_Dtc);
    _dtc_i->fMacAddrByte    = _odb_i->GetDtcMacAddrByte(H_Dtc);
    _dtc_i->fEmulateCfo     = _odb_i->GetDtcEmulatesCfo(H_Dtc);
//-----------------------------------------------------------------------------
// use global sample edge mode from /Mu2e/ActiveRunConfiguration/DAQ/ForceCFOSampleEdgeSelect
//-----------------------------------------------------------------------------
    // _dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(H_Dtc);
    HNDLE h_daq             = _odb_i->GetDaqConfigHandle(H_RunConf);
    _dtc_i->fSampleEdgeMode = _odb_i->GetInteger(h_daq,"ForceCfoSampleEdgeSelect");
    
    _dtc_i->fEventMode      = _odb_i->GetEventMode        (H_RunConf);
    _dtc_i->fRocReadoutMode = _odb_i->GetRocReadoutMode   (H_RunConf);
    _dtc_i->fJAMode         = _odb_i->GetJAMode           (H_Dtc);
    _dtc_i->fRocLaneMask    = _odb_i->GetUInt32           (H_Dtc,"RocLaneMask");

    //    _dtc_i->fDtcEwmDelay5ns = _odb_i->GetInteger          (H_Dtc,"ewm_delay_5ns");

    // _dtc_i->fDigitizationStart5ns = _odb_i->GetInteger    (h_daq,"digitization_start_5ns");
    // _dtc_i->fDigitizationStop5ns  = _odb_i->GetInteger    (h_daq,"digitization_stop_5ns" );

    TLOG(TLVL_DEBUG) << "subsystem:"         << subsystem
                     << std::format(" fw_version:0x{:08x}",fw_version)
                     << " _readout_mode:"    << std::dec << _dtc_i->fRocReadoutMode
                     << " roc_readout_mode:" << _dtc_i->fRocReadoutMode
                     << " sample_edge_mode:" << _dtc_i->fSampleEdgeMode
                     << " event_mode:"       << _dtc_i->fEventMode
                     << " emulate_cfo:"      << _dtc_i->fEmulateCfo
                     << std::format(" roc_lane_mask:0x{:04x}",_dtc_i->fRocLaneMask) ;
    //                     << std::format(" DTC ewm_delay_5ns:{}",_dtc_i->fDtcEwmDelay5ns) ;
//-----------------------------------------------------------------------------
// loop over the links, redefine the enabled link mask (also in ODB)
// also store in ODB IDs of the ROCs
//-----------------------------------------------------------------------------
    int mask = 0;
    for (int i=0; i<6; i++) {
      _dtc_i->SetLinkStatus(i,0);
      int link_enabled = _odb_i->GetLinkEnabled(H_Dtc,i);
      int link_locked  = _dtc_i->LinkLocked(i);
      TLOG(TLVL_DEBUG) << std::format("DTC{} link:{} enabled:{} locked:{} status:{}",
                                      _dtc_i->PcieAddr(),i,link_enabled,link_locked,_dtc_i->LinkStatus(i));
      if (not link_enabled)                               continue;
      if (not link_locked) {
        TLOG(TLVL_ERROR) << std::format("{}:DTC{} link:{} enabled but not locked, set link status to -1",
                                        HostLabel(),_dtc_i->PcieAddr(),i);
        _dtc_i->SetLinkStatus(i,-1);
        continue;
      }
      
      mask |= (1 << 4*i);

      std::string roc_id     ("READ_ERROR");
      std::string design_info("READ_ERROR");
      std::string git_commit ("READ_ERROR");
      
      try {
        roc_id      = _dtc_i->GetRocID         (i);
        TLOG(TLVL_DEBUG) << "roc_id:" << roc_id;
        design_info = _dtc_i->GetRocDesignInfo (i);
        TLOG(TLVL_DEBUG) << "design_info:" << design_info;
        git_commit  = _dtc_i->GetRocFwGitCommit(i);
        TLOG(TLVL_DEBUG) << "git_commit:" << git_commit;
      }
      catch(...) {
        TLOG(TLVL_ERROR) << std::format("{}:DTC{}: cant read link:{} ROC info, set link status to -1",HostLabel(),_dtc_i->PcieAddr(),i);
        _dtc_i->SetLinkStatus(i,-1);
        continue;
      }
//-----------------------------------------------------------------------------
// 'h_link' points to a subsystem-specific place
//-----------------------------------------------------------------------------
      char key[10];
      sprintf(key,"Link%d",i);
      HNDLE h_link = _odb_i->GetHandle(H_Dtc,key);
      _odb_i->SetRocID         (h_link,roc_id     );
      _odb_i->SetRocDesignInfo (h_link,design_info);
      _odb_i->SetRocFwGitCommit(h_link,git_commit );
      
      int roc_ewm_delay_5ns      = _odb_i->GetInteger(h_link,"ewm_delay_5ns");
      _dtc_i->fRocEwmDelay5ns[i] = roc_ewm_delay_5ns;
    }
//-----------------------------------------------------------------------------
// set link mask, also update link mask in ODB - that is not used, but is convenient
//-----------------------------------------------------------------------------
    _dtc_i->fLinkMask      = mask;
    _odb_i->SetLinkMask(H_Dtc,mask);
//-----------------------------------------------------------------------------
// write panel IDs - to begin with, make it a separate loop
// comment it out, already done by Vadim
//-----------------------------------------------------------------------------
//       trkdaq::ControlRoc_DigiRW_Input_t  pin;
//       trkdaq::ControlRoc_DigiRW_Output_t pout;
//       pin.rw      = 1;
//       pin.hvcal   = 0;
//       pin.address = 0x90;
//       for (int i=0; i<6; i++) {
//         int link_enabled = dtc_i->LinkEnabled(i);
//         TLOG(TLVL_DEBUG) << "link:" << i << " link_enabled:" << link_enabled;
//         if (link_enabled) {
// //-----------------------------------------------------------------------------
// // Minnesota panel name .. at this point 
// //-----------------------------------------------------------------------------
//           char key[32];
//           sprintf(key,"Link%d",i);
//           HNDLE h_link = _odb_i->GetHandle(h_subkey,key);
//           std::string panel_mn_name = _odb_i->GetString(h_link,"DetectorElement/Name");
//           int mn_id = atoi(panel_mn_name.substr(2).data());
//           // and write it to the digis
//           pin.data[0] = int16_t(mn_id);
//           pin.data[1] = 0;
//           TLOG(TLVL_DEBUG) << "link:" << i << " panel_mn_name:" << panel_mn_name
//                            << " mn_id:" << mn_id
//                            << " pin.data[0]:" << pin.data[0] << " pin.data[1]:" << pin.data[1];
// //-----------------------------------------------------------------------------
// // PM: make it look ugly - the ugliness indicates that the logic is not right
// // most likely, need different node frontends for different subsystems
// //-----------------------------------------------------------------------------
//           trkdaq::DtcInterface* trk_dtc_i = (trkdaq::DtcInterface*) dtc_i;
//           trk_dtc_i->ControlRoc_DigiRW(&pin,&pout,i);
//         }
//       }
//-----------------------------------------------------------------------------
// monitoring
//-----------------------------------------------------------------------------
    _monitoringLevel     = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/DTC"  );
    _monitorSPI          = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/SPI"  );
    _monitorRates        = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Rates");
    _monitorRocRegisters = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/RocRegisters");
  
    InitVarNames();
//-----------------------------------------------------------------------------
// hotlinks - start from one function handling both DTCs
// command processor : 'ProcessCommand' function
//-----------------------------------------------------------------------------
    HNDLE hdb       = _odb_i->GetDbHandle();
    HNDLE h_cmd     = _odb_i->GetDtcCmdHandle(_host_label,pcie_addr);
    HNDLE h_cmd_run = _odb_i->GetHandle(h_cmd,"Run");

    TLOG(TLVL_DEBUG) << "before db_open_record: h_cmd_run:" << h_cmd_run << " _cmd_run:" << _cmd_run;
    
    if (db_open_record(hdb,h_cmd_run,&_cmd_run,sizeof(int32_t),MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
      std::string msg = std::format("cannot open DTC{} hotlink in ODB",_dtc_i->PcieAddr());
      TLOG(TLVL_ERROR) << msg;
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
      SetStatus(-1);
    }
//-----------------------------------------------------------------------------
// if everything went well, init readout
//-----------------------------------------------------------------------------
    InitReadout(_cmd_handle);
  }
  
//2026-04-09 PM  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  _logfile             = std::format("{}/logs/{}_{}.log",data_dir,HostLabel(),_dtc_i->PcieAddr());

  TLOG(TLVL_DEBUG) << "-- END";
}


//-----------------------------------------------------------------------------
// can afford printing every time - once per run
//-----------------------------------------------------------------------------
int TEqTrkDtc::BeginRun(int RunNumber) {
  int rc(0);
    
  TLOG(TLVL_DEBUG) << std::format("-- START: host:{} DTC:{}" ,HostLabel(),_dtc_i->PcieAddr());
 
  int   run_type          = _odb_i->GetRunType       (_h_active_run_conf);
  int   event_mode        = _odb_i->GetEventMode     (_h_active_run_conf);
  int   roc_readout_mode  = _odb_i->GetRocReadoutMode(_h_active_run_conf);

  TLOG(TLVL_DEBUG) << std::format("run_type:{} event mode:{} roc_readout_mode:{}",run_type,event_mode,roc_readout_mode);
  
  if (_dtc_i) {
                                        // update parameters from ODB
    _dtc_i->fEventMode      = event_mode;
    _dtc_i->fRocReadoutMode = roc_readout_mode;
    _dtc_i->fLinkMask       = _odb_i->GetLinkMask         (_handle);
    _dtc_i->fJAMode         = _odb_i->GetJAMode           (_handle);
//-----------------------------------------------------------------------------
// sample edge select is common for all DTCs and comes from DAQ/ForceCfoSampleEdgeSelect
    _dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(_h_active_run_conf);
//-----------------------------------------------------------------------------
// at begin run, initialize the readout
//-----------------------------------------------------------------------------
    if (_dtc_i->fSubsystem == mu2edaq::kTracker) {
      InitReadout(_cmd_handle);
    }
  }
  
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;
  return rc;
}
//-----------------------------------------------------------------------------
// in the end of run, read out ROC registers and dump them
// for now, dump $DAQ_OUTPUT_TOP/logs/node_frontend/{:6d}_registers.txt
//-----------------------------------------------------------------------------
int TEqTrkDtc::EndRun(int RunNumber) {
  int rc(0);

    
  TLOG(TLVL_DEBUG) << "-- START: DTC" << _dtc_i->PcieAddr() << ":" << _dtc_i;
  TLOG(TLVL_DEBUG) << "-- END ... do nothing ... rc:" << rc;

  return 0;
  
  SetStatus(1);

  std::vector<uint32_t>  dtc_reg;
  dtc_reg.reserve(DtcRegisters.size());
        
  for (const int reg : DtcRegisters) {
    uint32_t dat(0);
    try {
      _dtc_i->fDtc->GetDevice()->read_register(reg,100,&dat);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "failed to read register:" << reg;
      dat = 0xFFFFFFFF;
    }
    dtc_reg.emplace_back(dat);
  }

  std::vector<uint32_t> roc_reg[6];

  for (int lnk=0; lnk<6; lnk++) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue;
    if (_dtc_i->LinkEnabled(lnk) == 0) continue;
    rc = ReadRocRegisters(lnk,RocRegisters,roc_reg[lnk]);
    if (rc < 0) {
      SetStatus(rc);
      TLOG(TLVL_ERROR) << std::format("-- END BAILING OUT, rc:{}",rc);
      return rc;
    }
  }

//-----------------------------------------------------------------------------
// just save it
//-----------------------------------------------------------------------------
  std::mutex mtx; // keep it thread-safe .. why ?

  {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream output_file;

    const char* daq_output_top = getenv("DAQ_OUTPUT_TOP");
    
    std::string fn = std::format("{}/logs/node_frontend/{:06d}_registers_dtc{}.txt",daq_output_top,RunNumber,_dtc_i->PcieAddr());
    
    output_file.open(fn.data(),std::ofstream::trunc);
    if (not output_file.is_open()) {
      TLOG(TLVL_ERROR) << std::format("failed to open _logfile:{} in ofstream::trunc mode",fn); 
    }
    else {
//-----------------------------------------------------------------------------
// file opened OK, start from DTC registers
//-----------------------------------------------------------------------------
      int n_dtc_regs = DtcRegisters.size();
      for (int i=0; i<n_dtc_regs; ++i) {
        int reg = DtcRegisters.begin()[i];
        output_file << std::format("DTC reg:0x{:04x} : 0x{:04x}\n",reg,dtc_reg[i]);
      }
//-----------------------------------------------------------------------------
// now ROC registers
//-----------------------------------------------------------------------------
      int n_roc_regs = RocRegisters.size();
      for (int i=0; i<n_roc_regs; ++i) {
        output_file << std::format("ROC reg:{:4d} :",RocRegisters.begin()[i]);
        for (int lnk=0; lnk<6; lnk++) {
          if ((_dtc_i->LinkEnabled(lnk) == 0) or (_dtc_i->LinkEnabled(lnk) == 0)) {
            output_file << "      ";
          }
          else {
            std::vector<uint32_t>* rr = &roc_reg[lnk];
            output_file << std::format(" 0x{:04x}",rr->at(i));
          }
        }
        output_file << std::endl;
      }
      
      output_file.close();
    }
    
    SetStatus(rc);
  }

  
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;
  return rc;
}

//-----------------------------------------------------------------------------
TMFeResult TEqTrkDtc::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int TEqTrkDtc::InitVarNames() {

  TLOG(TLVL_DEBUG) << "-- START HostLabel:" << HostLabel();
  
  // SC TODO: I think we should define these together with the registers? 
  std::initializer_list<const char*> dtc_names = {
    "Temp", "VCCINT", "VCCAUX", "VCBRAM"
  };
  
  // std::initializer_list<const char*> dtr_names = {
  //   "R9650", "R9654", "R9658" , "R965c", "R9660",  // TX HB registers
  //   "R9664", "R9668"
  //  };

  const std::string eq_path       = "/Equipment/"+HostLabel();
  const std::string settings_path = eq_path+"/Settings";

  midas::odb        odb_settings(settings_path);   // points to /Settings

  int pcie_addr = _dtc_i->PcieAddr();
  
  std::vector<std::string> dtc_var_names;
  for (const char* name : dtc_names) {
    std::string var_name = std::format("dtc{}#{:s}",pcie_addr,name);
    dtc_var_names.push_back(var_name);
  }

  std::string dirname = std::format("Names dtc{:d}",pcie_addr);
  odb_settings[dirname] = dtc_var_names;
//-----------------------------------------------------------------------------
// DTC counters and such
//-----------------------------------------------------------------------------
  dtc_var_names.clear();
  for (const int& reg : DtcRegisters) {
    std::string var_name = std::format("dtc{}#r_0x{:04x}",pcie_addr,reg);
    dtc_var_names.push_back(var_name);
  }

  for (int i=15; i<21; i++) {
    std::string var_name = std::format("dtc{}#link{}-cfo",pcie_addr,i-15);
    dtc_var_names.push_back(var_name);
  }
      
  dirname = std::format("Names dtr{:d}",pcie_addr);
  odb_settings[dirname] = dtc_var_names;
//-----------------------------------------------------------------------------
// noise rates
//-----------------------------------------------------------------------------
  std::vector<std::string> rate_var_names;
  for (int i=0; i<6; i++) {
    std::string var_name = std::format("dtc{}#link{}",pcie_addr,i);
    rate_var_names.push_back(var_name);
  }

  dirname = std::format("Names dtc{}_rates",pcie_addr);
  odb_settings[dirname] = rate_var_names;
//-----------------------------------------------------------------------------
// loop over the ROCs and create names for each of them
// add to the ROC (per-panel) data the key and the ILP (pressure/temp) readout
//-----------------------------------------------------------------------------
  for (int ilink=0; ilink<6; ilink++) { 
     
    std::vector<std::string> roc_var_names;
    for (int k=0; k<trkdaq::TrkSpiDataNWords; k++) {
      char var_name[32];
      sprintf(var_name,"rc%i%i#%s",pcie_addr,ilink,trkdaq::DtcInterface::SpiVarName(k));
      roc_var_names.push_back(var_name);
    }
      
    for (int k=0; k<trkdaq::TrkKeyDataNWords; k++) {
      char var_name[32];
      sprintf(var_name,"rc%i%i#%s",pcie_addr,ilink,trkdaq::DtcInterface::KeyVarName(k));
      roc_var_names.push_back(var_name);
    }
      
    for (int k=0; k<trkdaq::TrkIlpDataNWords; k++) {
      char var_name[32];
      sprintf(var_name,"rc%i%i#%s",pcie_addr,ilink,trkdaq::DtcInterface::IlpVarName(k));
      roc_var_names.push_back(var_name);
    }
      
    dirname = std::format("Names rc{}{}",pcie_addr,ilink);
    if (not midas::odb::exists(settings_path+"/"+dirname)) {
      odb_settings[dirname] = roc_var_names;
    }
//-----------------------------------------------------------------------------
// non-history ROC registers - counters and such - just to be looked at
//-----------------------------------------------------------------------------
    roc_var_names.clear();
    for(const int& reg : RocRegisters) {
      char var_name[32];
      sprintf(var_name,"reg_%03i",reg);
      roc_var_names.push_back(var_name);
    }

    char roc_subdir[128];

    sprintf(roc_subdir,"%s/DTC%i/ROC%i",eq_path.data(),pcie_addr,ilink);
    midas::odb odb_roc = {{"RegName",{"a","b"}},{"RegData",{1u,2u}}};
    odb_roc.connect(roc_subdir);
    odb_roc["RegName"] = roc_var_names;
    
    std::vector<uint16_t> roc_reg_data(roc_var_names.size());
    odb_roc["RegData"] = roc_reg_data;
  }
  
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
void TEqTrkDtc::ReadNonHistRegisters() {
  
  std::string node_var_path = std::format("/Equipment/{}/Variables",HostLabel());

  std::vector<uint32_t>  dtc_reg;
  dtc_reg.reserve(DtcRegisters.size());
        
  for (const int reg : DtcRegisters) {
    uint32_t dat(0);
    try {
      _dtc_i->fDtc->GetDevice()->read_register(reg,100,&dat);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "failed to read register:" << reg;
      dat = 0xFFFFFFFF;
    }
    dtc_reg.emplace_back(dat);
  }
  // should be 22 words

  for (int i=15; i<21; i++) {
    uint32_t x = dtc_reg[21]-dtc_reg[i];        // should be non-negative
    dtc_reg.emplace_back(x);
  }
  // 22+6 = 28 words
  
  std::string record_name = std::format("dtr{}",_dtc_i->PcieAddr());
      
  //  TLOG(TLVL_DEBUG+1) << "N(DTC registers):" << DtcRegisters.size() << " buf:" << buf;
        
  midas::odb xx = {{record_name.data(),{1u}}};
  xx.connect(node_var_path);
  xx[record_name].resize(dtc_reg.size());
  xx[record_name] = dtc_reg;

  TLOG(TLVL_DEBUG+1) << "-- END";
}

//-----------------------------------------------------------------------------
// Link ne -1...
//------------------------------------------------------------------------------
int TEqTrkDtc::ReadRocRegisters(int Link, const std::vector<int>& Registers, std::vector<uint32_t>& RegData) {
  int rc(0);

  RegData.clear();
  RegData.reserve(Registers.size());

  for (const int reg : RocRegisters) {
    // ROC registers store 16-bit words, don't know how to declare an array
    // of shorts for ODBXX, use uint32_t
    try {
      uint32_t dat = _dtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(Link),reg,100); 
      RegData.emplace_back(dat);
    }
    catch(...) {
      rc = -1;
      TLOG(TLVL_ERROR) << "failed to read DTC:" << _dtc_i->PcieAddr() << " ROC:" << Link << " registers";
      SetStatus(-1);
      break;
    }
  }
  return rc;
}

//-----------------------------------------------------------------------------
// is called only if ::MonitoringLevel() > 0
//-----------------------------------------------------------------------------
int TEqTrkDtc::HandlePeriodic() {
  int rc(0);
  TLOG(TLVL_DEBUG+1) << std::format("-- START: host:{} DTC:{}",HostLabel(),_dtc_i->PcieAddr());
//-----------------------------------------------------------------------------
// DTC temperature and voltages - for history 
//-----------------------------------------------------------------------------
  int pcie_addr = _dtc_i->PcieAddr();
  
  midas::odb o_runinfo("/Runinfo");
  int running_state          = o_runinfo["State"];
  int transition_in_progress = o_runinfo["Transition in progress"];

  try {
    std::vector<float> dtc_tv;
    for (const int reg : DtcRegHist) {
      uint32_t val;
      float    fval(-1.);
      try { 
        _dtc_i->fDtc->GetDevice()->read_register(reg,100,&val); 
        if      ( reg == 0x9010) fval = (val/4096.)*503.975 - 273.15;   // temperature
        else                     fval = (val/4095.)*3.;                 // voltage
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "failed to read register:" << reg;
        fval = -1;
      }
      
      dtc_tv.emplace_back(fval);
    }
    
    char dtc_name[16];
    sprintf(dtc_name,"dtc%i",pcie_addr);
    midas::odb odb_dtc_tv = {{dtc_name,{1.0f, 1.0f, 1.0f, 1.0f}}};

    std::string node_eq_path  = std::format("/Equipment/{}",HostLabel());
    std::string node_var_path = std::format("/Equipment/{}/Variables",HostLabel());

    odb_dtc_tv.connect(node_var_path);    
    odb_dtc_tv[dtc_name] = dtc_tv;
//-----------------------------------------------------------------------------
// non-history registers : 'dtr' = "DTcRegisters"
//-----------------------------------------------------------------------------
    ReadNonHistRegisters();
//-----------------------------------------------------------------------------
// for each enabled DTC, loop over its ROCs and read ROC registers
// this part can depend on on the type of the ROC
// do it for the tracker
// don't use 'link' - ROOT doesn't like 'link' for a variable name
//-----------------------------------------------------------------------------
    std::vector<float> total_rate(6,-1.);
    
    for (int ilink=0; ilink<6; ilink++) {
      
      TLOG(TLVL_DEBUG+1) << std::format("link:{} enabled:{} locked:{} status:{}",
                                        ilink,_dtc_i->LinkEnabled(ilink),_dtc_i->LinkLocked(ilink),_dtc_i->LinkStatus(ilink));
                                        
      if (_dtc_i->LinkEnabled(ilink) == 0) continue;
                                        // skip links which status has been set to -1
      if (_dtc_i->LinkStatus(ilink)  != 0) continue;
      if (not _dtc_i->LinkLocked(ilink)) {
        std::string msg = std::format("{}:DTC{} link:{} enabled but not locked, set link status to -1",
                                      HostLabel(),_dtc_i->PcieAddr(),ilink);
        TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
        cm_msg(MERROR, __func__,msg.data());
        cm_msg_flush_buffer();
                                        // links with status < 0 should be displayed in red and skipped w/o extra messaging -
                                        // a message has been sent once when the link status has been changed
        _dtc_i->SetLinkStatus(ilink,-1);
        SetStatus(-1);
        continue;
      }
      
      TLOG(TLVL_INFO) << std::format("{}:DTC{} link:{} : proceed with the monitoring: roc_regs:{} spi:{} rates:{}",
                                     HostLabel(),_dtc_i->PcieAddr(),ilink,_monitorRocRegisters,_monitorSPI,_monitorRates);
      
      if (_monitorRocRegisters > 0) {
        
        std::vector<uint32_t>  roc_reg;
        roc_reg.reserve(RocRegisters.size());
            
        rc = ReadRocRegisters(ilink,RocRegisters,roc_reg);

        if (rc == 0) {
          char buf[100];
          sprintf(buf,"%s/DTC%i/ROC%i",node_eq_path.data(),_dtc_i->PcieAddr(),ilink);
          
          midas::odb roc = {{"RegData",{1u}}};
          roc.connect(buf);
          roc["RegData"] = roc_reg;
        }
      }
//-----------------------------------------------------------------------------
// SPI - an ODB change should be sufficient
//-----------------------------------------------------------------------------
      _monitorSPI          = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/SPI"  );
      if (_monitorSPI > 0) {
        TLOG(TLVL_DEBUG+1) << "saving ROC:" << ilink << " SPI data";
        
        struct trkdaq::TrkSpiData_t   spi;
        int rc = _dtc_i->ControlRoc_ReadSpi_1(&spi,ilink,0);
        if (rc == 0) {
              
          std::vector<float> roc_spi;
          for (int iw=0; iw<trkdaq::TrkSpiDataNWords; iw++) {
            roc_spi.emplace_back(spi.Data(iw));
          }
//-----------------------------------------------------------------------------
// read key data
//-----------------------------------------------------------------------------
          std::vector<uint16_t> key_data;
          int print_level(0);
          rc = _dtc_i->ControlRoc_GetKey(key_data,ilink,print_level);
          if (rc == 0) {
            // for now, do it 'brute force' way, improve later
            float temp     = float(key_data[0])/4096.*3300./10;
            float v2p5     = float(key_data[1])/4096*3.355;
            float v5p1     = float(key_data[2])/4096.*3.355*2;
            float dcdctemp = float(key_data[3])/4096*3300/10;

            roc_spi.emplace_back(temp);
            roc_spi.emplace_back(v2p5);
            roc_spi.emplace_back(v5p1);
            roc_spi.emplace_back(dcdctemp);
          }
          else {
            for (int i=0; i<trkdaq::TrkKeyDataNWords; i++) roc_spi.emplace_back(-1.);
          }
//-----------------------------------------------------------------------------
// read ILP data
//-----------------------------------------------------------------------------
          std::vector<uint16_t> ilp_data;
          rc = _dtc_i->ControlRoc_ReadIlp(ilp_data,ilink,print_level);
          if (rc == 0) {
            int   ilp_id   = ilp_data[0];
            float temp     = float(ilp_data[1])/100.;
            float pressure = float(int(ilp_data[3]) << 16 | int(ilp_data[2]))/524288.;
            roc_spi.emplace_back(float(ilp_id));
            roc_spi.emplace_back(temp);
            roc_spi.emplace_back(pressure);
          }
          else {
            for (int i=0; i<trkdaq::TrkIlpDataNWords; i++) roc_spi.emplace_back(-1.);
          }
          
          char buf[100];
          sprintf(buf,"rc%i%i",_dtc_i->PcieAddr(),ilink);
            
          midas::odb xx = {{buf,{1.0f}}};
          xx.connect(node_var_path);

          int nw = trkdaq::TrkSpiDataNWords+trkdaq::TrkKeyDataNWords+trkdaq::TrkIlpDataNWords;
          xx[buf].resize(nw);
          xx[buf] = roc_spi;
              
          TLOG(TLVL_DEBUG+1) << std::format("host:{} DTC:{} link:{} : saved N(SPI+KEY+ILP) words:{}",
                                            HostLabel(),_dtc_i->PcieAddr(),ilink,nw);
        }
        else {
          std::string msg = std::format("host:{} DTC:{} link:{} : failed to read SPI, set link status to -1",
                                        HostLabel(),_dtc_i->PcieAddr(),ilink);
          TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
          cm_msg(MERROR, __func__,msg.data());
          cm_msg_flush_buffer();
//-----------------------------------------------------------------------------
// set ROC? DTC? (try DTC first) status to -1 and do not try to read it...
//-----------------------------------------------------------------------------
          SetLinkStatus(ilink,-1);
          SetStatus(-1);
        }
      }
//-----------------------------------------------------------------------------
// ROC rates
// for now, assume that the clock has been set to internal ,
// need to find the right place to set marker_clock to 0 (and may be recover in the end),
// will do it right later
// don't cache , get directrly from ODB
//-----------------------------------------------------------------------------
      int monitor_rates  = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Rates");
      if ((monitor_rates > 0) and (transition_in_progress == 0) and (running_state != STATE_RUNNING)) {
        TLOG(TLVL_DEBUG+1) << "MONITOR RATES link:" << ilink;
//-----------------------------------------------------------------------------
// for monitoring, want to read ALL channels.
// 1. run read command enabling the internal clock and setting the read mask to read all channels
// 2. run rates command with all channels enabled
// 3. run read command restoring the clock marker (source of the clock) and the read mask
//    which someone may rely on
//----------------------------------------------------------------------------
        midas::odb o_read_cmd;
        o_read_cmd.connect("/Mu2e/Commands/Tracker/read");

        trkdaq::ControlRoc_Read_Input_t0 pread;                // ch_mask is set to all oxffff
                                                               // save the read command ch_mask
        uint16_t saved_ch_mask[6];
        for (int i=0; i<6; ++i) saved_ch_mask[i] = o_read_cmd["ch_mask"][i];
            
        pread.adc_mode        = o_read_cmd["adc_mode"     ];   // -a
        pread.tdc_mode        = o_read_cmd["tdc_mode"     ];   // -t 
        pread.num_lookback    = o_read_cmd["num_lookback" ];   // -l 
        
        pread.num_samples     = o_read_cmd["num_samples"  ];   // -s
        pread.num_triggers[0] = o_read_cmd["num_triggers"][0]; // -T 10
        pread.num_triggers[1] = o_read_cmd["num_triggers"][1]; //
//-----------------------------------------------------------------------------
// this is a tricky place: rely on that the READ command ODB record
// stores the -p value used during the data taking
//-----------------------------------------------------------------------------
        pread.enable_pulser   = o_read_cmd["enable_pulser"];   // -p 1
        pread.marker_clock    = 0;                             // to read the rates, enable internal clock
        pread.mode            = o_read_cmd["mode"         ];   // 
        pread.clock           = o_read_cmd["clock"        ];   //
          
        int print_level       = 0;

        TLOG(TLVL_DEBUG+1) << std::format("before ControlRoc_Read");
                                          
        _dtc_i->ControlRoc_Read(&pread,ilink,print_level);

        std::vector<uint16_t> rates;
        trkdaq::ControlRoc_Rates_t* par(nullptr); // defaults are OK - read all channels
        std::ostream null_stream(nullptr);
        
        TLOG(TLVL_DEBUG+1) << std::format("node:{} DTC:{} link:{} before reading rates",HostLabel(),_dtc_i->PcieAddr(),ilink);
        int rc = _dtc_i->ControlRoc_Rates(ilink,&rates,print_level,par,null_stream);
        TLOG(TLVL_DEBUG+1) << std::format("node:{} DTC:{} link:{} after reading rates",HostLabel(),_dtc_i->PcieAddr(),ilink);
//-----------------------------------------------------------------------------
// and restore the READ command mask and the clock
//-----------------------------------------------------------------------------
        pread.marker_clock    = o_read_cmd["marker_clock" ];   // restore the marker_clock mode
        for (int i=0; i<6; ++i) pread.ch_mask[i] = saved_ch_mask[i];
        _dtc_i->ControlRoc_Read(&pread,ilink,print_level);
//-----------------------------------------------------------------------------
// in ODB, store the total coincidence rate , on number per ROC
//-----------------------------------------------------------------------------
        if (rc == 0) {
 //-----------------------------------------------------------------------------
// finally, the last two words - total counts
//-----------------------------------------------------------------------------
          float total[2];          // [0]:CAL  [1]:HV , as in lanes, an inversion takes place
          float clock_tick(5.e-9); // 5 ns <-> 200 MHz clock
      
          int loc = 576;   // = 96*6
    
          total[1]  = float(rates[loc  ])+(int(rates[loc+1]) << 16); // hv - check the order with Vadim
          total[0]  = float(rates[loc+2])+(int(rates[loc+3]) << 16); // cal

          uint16_t panel_ch_mask[96];
          std::string mask_odb_path = std::format("Link{:d}/DetectorElement/ch_mask",ilink);
          _odb_i->GetArray(_handle,mask_odb_path.data(),TID_WORD,panel_ch_mask,96);

          float trate = 0;
          float denom = (total[0]+total[1])/2.*clock_tick;
          for (int ich=0; ich<96; ich++) {
            if (panel_ch_mask[ich] == 0) continue;

            loc               = 6*ich;
            // int   counts_hv   = int((*Rates)[loc  ])+(int((*Rates)[loc+1]) << 16);
            // int   counts_cal  = int((*Rates)[loc+2])+(int((*Rates)[loc+3]) << 16);
            int   counts_coin  = int(rates[loc+4])+(int(rates[loc+5]) << 16);
            // int   ifpga        = trkdaq::DtcInterface::DigiFpga(ich);
            // float rate_hv     = counts_hv /total[ifpga]/clock_tick/1000.;
            // float rate_cal    = counts_cal/total[ifpga]/clock_tick/1000.;
            trate               += counts_coin;
          }
          total_rate[ilink] = trate/denom/1.e3;  // kHz
          TLOG(TLVL_ERROR) << "failed to read rates DTC:" << _dtc_i->PcieAddr() << " ROC:" << ilink;
        }
        else {
          TLOG(TLVL_ERROR) << "failed to read rates DTC:" << _dtc_i->PcieAddr() << " ROC:" << ilink;
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
          // TODO
        }
      }
    }

    char buf[16];
    sprintf(buf,"dtc%d_rates",_dtc_i->PcieAddr());
          
    midas::odb vars(node_var_path);
    vars[buf] = total_rate;
    TLOG(TLVL_DEBUG+1) << std::format(" saved rates: {} {} {} {} {} {} to {}" ,
                                      total_rate[0],total_rate[1],total_rate[2],total_rate[3],total_rate[4],total_rate[5],
                                      node_var_path);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "failed to read DTC:" << _dtc_i->PcieAddr() << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
    SetStatus(-1);
    // TODO
  }

  TLOG(TLVL_DEBUG+1) << "-- END";
  return rc;
}

//------------------------------------------------------------------------------
// initialization of various run types
//-----------------------------------------------------------------------------
int TEqTrkDtc::InitBeamRun() {
  int rc(-1);
  TLOG(TLVL_ERROR) << std::format("not implemented yet");
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::InitCosmicRun() {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("--START");
//-----------------------------------------------------------------------------
// 1. set the data readout mode .. the pattern should be stroed in ODB, not hardcoded
//   everything except the channel mask should be the same for all panels
//-----------------------------------------------------------------------------
  trkdaq::ControlRoc_Read_Input_t0 par;

  std::string parameter_path("/Mu2e/ActiveRunConfiguration/Tracker/Readout/cosmics/read");
  HNDLE h_par = _odb_i->GetHandle(0,parameter_path);
  
  par.adc_mode        = _odb_i->GetUInt16(h_par,"adc_mode");              // 0:data, 4:checkerboard, etc
  par.tdc_mode        = _odb_i->GetUInt16(h_par,"tdc_mode");
  par.num_lookback    = _odb_i->GetUInt16(h_par,"num_lookback");             // for 1-packet RO
  par.num_samples     = _odb_i->GetUInt16(h_par,"num_samples");              // N ADC samples - may need to change... - ODB
  
  _odb_i->GetArray(h_par,"num_triggers",TID_WORD,par.num_triggers,2);

  par.enable_pulser   = _odb_i->GetUInt16(h_par,"enable_pulser");
  par.marker_clock    = _odb_i->GetUInt16(h_par,"marker_clock");
  par.mode            = _odb_i->GetUInt16(h_par,"mode");
  par.clock           = _odb_i->GetUInt16(h_par,"clock");
//-----------------------------------------------------------------------------
// loop over the ROCs and set internal pulser mode
//-----------------------------------------------------------------------------
  for (int lnk=0; lnk<6; lnk++) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue;
    // skip links which status has been set to -1
    if (_dtc_i->LinkStatus(lnk)  != 0) continue;
    if (not _dtc_i->LinkLocked(lnk)) {
      std::string msg = std::format("{}:DTC{} link:{} enabled but not locked, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
                                        // links with status < 0 should be displayed in red and skipped w/o extra messaging -
                                        // a message has been sent once when the link status has been changed
      _dtc_i->SetLinkStatus(lnk,-1);
      SetStatus(-1);
      continue;
    }
//-----------------------------------------------------------------------------
// link enabled, locked, and is OK in all known respeccts (status=0)
// 0. turn off the external pulser
// pulser OFF just doesn't work
//-----------------------------------------------------------------------------
    // rc = _dtc_i->ControlRoc_PulserOff(lnk);
    // if (rc < 0) {
    //   std::string msg = std::format("{}:DTC{} link:{} failed to turn the pulser OFF, set link status to -1",
    //                                 HostLabel(),_dtc_i->PcieAddr(),lnk);
    //   TLOG(TLVL_ERROR) << msg;
    //                                     // send message to MIDAS
    //   cm_msg(MERROR, __func__,msg.data());
    //   cm_msg_flush_buffer();
    //   SetStatus(-1);
    //   TLOG(TLVL_ERROR) << msg;
    //   continue;
    // }
//-----------------------------------------------------------------------------
// the channel mask comes from ODB - from the corresponding ROC
//-----------------------------------------------------------------------------
    uint16_t ch_mask[96];
    std::string  mask_odb_path = std::format("Link{:d}/DetectorElement/ch_mask",lnk);
    _odb_i->GetArray(_handle,mask_odb_path.data(),TID_WORD,ch_mask,96);    

    for (int i=0; i<96; ++i) {
      int on_off = ch_mask[i];
      int iw = i / 16;
      int ib = i % 16;
      if (ib == 0) {
        par.ch_mask[iw] = 0;
      }
      par.ch_mask[iw] |= on_off << ib;
    }
    // sstr << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
//-----------------------------------------------------------------------------
// 1. issue the READ command
//-----------------------------------------------------------------------------
    int print_level = 0;
    rc = _dtc_i->ControlRoc_Read(&par,lnk,print_level);
    if (rc < 0) {
      std::string msg = std::format("{}:DTC{} link:{} ControlRoc_Read failed, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
      SetStatus(-1);
      TLOG(TLVL_ERROR) << msg;
      continue;
    }
  }
  
  TLOG(TLVL_DEBUG) << std::format("--END: rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::InitInternalPulserRun() {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("--START");
//-----------------------------------------------------------------------------
// 1. set the readout mode .. the pattern should be stroed in ODB, not hardcoded
// the same for all DTCs
//-----------------------------------------------------------------------------
  trkdaq::ControlRoc_Read_Input_t0 par;  //

  std::string parameter_path("/Mu2e/ActiveRunConfiguration/Tracker/Readout/internal_pulser/read");
  HNDLE h_par         = _odb_i->GetHandle(0,parameter_path);
  
  par.adc_mode        = _odb_i->GetUInt16(h_par,"adc_mode");              // 0:data, 4:checkerboard, etc
  par.tdc_mode        = _odb_i->GetUInt16(h_par,"tdc_mode");
  par.num_lookback    = _odb_i->GetUInt16(h_par,"num_lookback");             // for 1-packet RO
  par.num_samples     = _odb_i->GetUInt16(h_par,"num_samples");              // N ADC samples - may need to change... - ODB
  
  _odb_i->GetArray(h_par,"num_triggers",TID_WORD,par.num_triggers,2);

  par.enable_pulser   = _odb_i->GetUInt16(h_par,"enable_pulser");
  par.marker_clock    = _odb_i->GetUInt16(h_par,"marker_clock");
  par.mode            = _odb_i->GetUInt16(h_par,"mode");
  par.clock           = _odb_i->GetUInt16(h_par,"clock");

  TLOG(TLVL_DEBUG) << std::format("DTC:{} adc_mode:{} enable_pulser:{} num_lookback:{}",
                                  _dtc_i->PcieAddr(),par.adc_mode,par.enable_pulser,par.num_lookback);
//-----------------------------------------------------------------------------
// loop over the ROCs and set internal pulser mode
//-----------------------------------------------------------------------------
  for (int lnk=0; lnk<6; lnk++) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue;
    // skip links which status has been set to -1
    if (_dtc_i->LinkStatus(lnk)  != 0) continue;
    if (not _dtc_i->LinkLocked(lnk)) {
      std::string msg = std::format("{}:DTC{} link:{} enabled but not locked, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
                                        // links with status < 0 should be displayed in red and skipped w/o extra messaging -
                                        // a message has been sent once when the link status has been changed
      _dtc_i->SetLinkStatus(lnk,-1);
      SetStatus(-1);
      continue;
    }
//-----------------------------------------------------------------------------
// link enabled, locked, and is OK in all known respects (status=0)
// 0. turn off the external pulser
//-----------------------------------------------------------------------------
    // TLOG(TLVL_DEBUG) << std::format("lnk:{} setting external pulser off",lnk);
    // rc = _dtc_i->ControlRoc_PulserOff(lnk);
    // TLOG(TLVL_DEBUG) << std::format("after pulser_off rc:{}",rc);
    // if (rc < 0) {
    //   std::string msg = std::format("{}:DTC{} link:{} failed to turn the pulser OFF, set link status to -1",
    //                                 HostLabel(),_dtc_i->PcieAddr(),lnk);
    //   TLOG(TLVL_ERROR) << msg;
    //                                     // send message to MIDAS
    //   cm_msg(MERROR, __func__,msg.data());
    //   cm_msg_flush_buffer();
    //   SetStatus(-1);
    //   TLOG(TLVL_ERROR) << msg;
    //   continue;
    // }
//-----------------------------------------------------------------------------
// the channel mask comes from ODB - from the parameters - need the same channels for all panels
//-----------------------------------------------------------------------------
    _odb_i->GetArray(h_par,"ch_mask",TID_WORD,par.ch_mask,6);
    TLOG(TLVL_DEBUG) << std::format("DTC:{} link:{} after reading ch_mask: 0x{:04x} 0x{:04x} 0x{:04x} 0x{:04x} 0x{:04x} 0x{:04x}"
                                    ,_dtc_i->PcieAddr(),lnk,
                                    par.ch_mask[0],par.ch_mask[1],par.ch_mask[2],par.ch_mask[3],par.ch_mask[4],par.ch_mask[5]);
//-----------------------------------------------------------------------------
// 1. issue the READ command
//-----------------------------------------------------------------------------
    int print_level = 3;
    TLOG(TLVL_DEBUG) << std::format("DTC:{} link:{} before ControRoc_Read: par.enable_pulser:{}",
                                    _dtc_i->PcieAddr(),lnk,par.enable_pulser);

    std::stringstream sout;
    rc = _dtc_i->ControlRoc_Read(&par,lnk,print_level,sout);
    TLOG(TLVL_DEBUG) << sout.str();
    
    TLOG(TLVL_DEBUG) << std::format("DTC:{} link:{} after ControRoc_Read",_dtc_i->PcieAddr(),lnk);
    
    if (rc < 0) {
      std::string msg = std::format("{}:DTC{} link:{} ControlRoc_Read failed, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
      SetStatus(-1);
      TLOG(TLVL_ERROR) << msg;
      continue;
    }
  }
  TLOG(TLVL_DEBUG) << std::format("--END: rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::InitPulseInjectionRun() {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("--START");
//-----------------------------------------------------------------------------
// 1. set the data readout mode .. should be stored in ODB, not hardcoded...
// the same for all DTCs
//-----------------------------------------------------------------------------
  std::string read_parameter_path("/Mu2e/ActiveRunConfiguration/Tracker/Readout/pulse_injection/read");
  HNDLE h_read         = _odb_i->GetHandle(0,read_parameter_path);
  
  trkdaq::ControlRoc_Read_Input_t0 par;
  par.adc_mode        = _odb_i->GetUInt16(h_read,"adc_mode");                 // 0:data, 4:checkerboard, etc
  par.tdc_mode        = _odb_i->GetUInt16(h_read,"tdc_mode");
  par.num_lookback    = _odb_i->GetUInt16(h_read,"num_lookback");             // for 1-packet RO
  par.num_samples     = _odb_i->GetUInt16(h_read,"num_samples");              // N ADC samples - may need to change... - ODB
  
  _odb_i->GetArray(h_read,"num_triggers",TID_WORD,par.num_triggers,2);

  par.enable_pulser   = _odb_i->GetUInt16(h_read,"enable_pulser");
  par.marker_clock    = _odb_i->GetUInt16(h_read,"marker_clock");
  par.mode            = _odb_i->GetUInt16(h_read,"mode");
  par.clock           = _odb_i->GetUInt16(h_read,"clock");

  TLOG(TLVL_DEBUG) << std::format("adc_mode:{} enable_pulser:{} num_lookback:{}",par.adc_mode,par.enable_pulser,par.num_lookback);
//-----------------------------------------------------------------------------
// pulser parameters
//-----------------------------------------------------------------------------
  std::string pulser_on_parameter_path("/Mu2e/ActiveRunConfiguration/Tracker/Readout/pulse_injection/pulser_on");
  HNDLE h_pon_par        = _odb_i->GetHandle(0,pulser_on_parameter_path);

  int first_channel_mask = _odb_i->GetInteger(h_pon_par,"first_channel_mask");    //
  int duty_cycle         = _odb_i->GetInteger(h_pon_par,"duty_cycle"        );    //
  int pulser_delay       = _odb_i->GetInteger(h_pon_par,"pulser_delay"      );    //

  TLOG(TLVL_DEBUG) << std::format("first_channel_mask:{} duty_cycle:{} pulser_delay:{}",first_channel_mask,duty_cycle,pulser_delay);
//-----------------------------------------------------------------------------
// loop over the ROCs and set pulse injection mode
//-----------------------------------------------------------------------------
  for (int lnk=0; lnk<6; lnk++) {

    TLOG(TLVL_DEBUG) << std::format("link:{} enabled:{} status:{} locked:{}",lnk,_dtc_i->LinkEnabled(lnk),_dtc_i->LinkStatus(lnk), _dtc_i->LinkLocked(lnk));
    
    if (_dtc_i->LinkEnabled(lnk) == 0) continue;
    // skip links which status has been set to -1
    if (_dtc_i->LinkStatus(lnk)  != 0) continue;
    if (not _dtc_i->LinkLocked(lnk)) {
      std::string msg = std::format("{}:DTC{} link:{} enabled but not locked, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
                                        // links with status < 0 should be displayed in red and skipped w/o extra messaging -
                                        // a message has been sent once when the link status has been changed
      _dtc_i->SetLinkStatus(lnk,-1);
      SetStatus(-1);
      continue;
    }

    int print_level = 0;
//-----------------------------------------------------------------------------
// in the pulse injection mode, the channel mask comes from the ODB
//------------------------------------------------------------------
    uint16_t ch_mask[96];
    std::string  mask_odb_path = std::format("Link{:d}/DetectorElement/ch_mask",lnk);
    _odb_i->GetArray(_handle,mask_odb_path.data(),TID_WORD,ch_mask,96);    

    for (int i=0; i<96; ++i) {
      int on_off = ch_mask[i];
      int iw = i / 16;
      int ib = i % 16;
      if (ib == 0) {
        par.ch_mask[iw] = 0;
      }
      par.ch_mask[iw] |= on_off << ib;
    }
    // sstr << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
    rc = _dtc_i->ControlRoc_Read(&par,lnk,print_level);

    TLOG(TLVL_DEBUG) << std::format("after ControlRoc_Read rc:{}",rc);
    
    if (rc < 0) {
      std::string msg = std::format("{}:DTC{} link:{} ControlRoc_Read failed, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
      SetStatus(-1);
      TLOG(TLVL_ERROR) << msg;
      continue;
    }
//-----------------------------------------------------------------------------
// link enabled, locked, and is OK in all known respeccts (status=0)
// 0. turn off the external pulser
//-----------------------------------------------------------------------------
    std::stringstream sstr;
    rc = _dtc_i->ControlRoc_PulserOn(lnk,first_channel_mask,duty_cycle,pulser_delay,print_level,sstr);
    TLOG(TLVL_DEBUG) << std::format("after ControlRoc_PulserOn rc:{}",rc);
    if (rc < 0) {
      std::string msg = std::format("{}:DTC{} link:{} failed to turn the pulser OFF, set link status to -1",
                                    HostLabel(),_dtc_i->PcieAddr(),lnk);
      TLOG(TLVL_ERROR) << msg;
                                        // send message to MIDAS
      cm_msg(MERROR, __func__,msg.data());
      cm_msg_flush_buffer();
      SetStatus(-1);
      TLOG(TLVL_ERROR) << msg;
      continue;
    }
  }

  TLOG(TLVL_DEBUG) << std::format("--END: rc:{}",rc);
  return rc;
}

//----------------------------------------------------------------------------------------
// it is helpful to redefine the monitoring level during the run - teh function is called rare enough
//----------------------------------------------------------------------------------------
int TEqTrkDtc::MonitoringLevel() {
  int level = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/DTC");
  return level;
}

//------------------------------------------------------------------------------
// set ODB status if a link 'Link'
//-----------------------------------------------------------------------------
void TEqTrkDtc::SetLinkStatus(int Link, int Status) {
  std::string link_status_path = std::format("Link{}/Status",Link);
  _odb_i->SetInteger(_handle,link_status_path.data(),Status);
}
