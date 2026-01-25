///////////////////////////////////////////////////////////////////////////////

#include "node_frontend/TEqCrvDtc.hh"
#include "utils/OdbInterface.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCrvDtc"
//-----------------------------------------------------------------------------
// 'skip_dtc_init' is common for all DTCs, may become obsolete
//-----------------------------------------------------------------------------
TEqCrvDtc::TEqCrvDtc(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Dtc) :
  TMu2eEqBase(Name,Title,TMu2eEqBase::kCrv)
{
  TLOG(TLVL_DEBUG) << "-- START: H_RunConf:" << H_RunConf << " H_Dtc:" << H_Dtc;

  _h_dtc               = H_Dtc;

  int dtc_enabled      = _odb_i->GetEnabled       (H_Dtc);
  int pcie_addr        = _odb_i->GetDtcPcieAddress(H_Dtc);
  int link_mask        = _odb_i->GetLinkMask      (H_Dtc);
  
  KEY key;
  _odb_i->GetKey(H_Dtc,&key);
  TLOG(TLVL_DEBUG) << "key.name:" << key.name;
  SetName(key.name);
//-----------------------------------------------------------------------------
// for now, disable the DTC re-initialization
//-----------------------------------------------------------------------------
  int skip_dtc_init    = _odb_i->GetSkipDtcInit   (H_RunConf);
  
  TLOG(TLVL_DEBUG) << "link_mask:0x" << std::hex << link_mask << " pcie_addr:" << pcie_addr;
  
  _dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
  std::string subsystem = _odb_i->GetString(H_Dtc,"Subsystem");
  _dtc_i->fSubsystem = mu2edaq::kCRV;
//-----------------------------------------------------------------------------
// start from checking the DTC FW verion and comparing it to the required one -
// defined in ODB
//-----------------------------------------------------------------------------
  uint32_t required_dtc_fw_version = _odb_i->GetDtcFwVersion(H_RunConf,subsystem.data());
  uint32_t dtc_fw_version          = _dtc_i->ReadRegister(0x9004);

  if ((required_dtc_fw_version != 0) and (dtc_fw_version != required_dtc_fw_version)) {
    TLOG(TLVL_ERROR) << "dtc_fw_version:" << std::hex << dtc_fw_version
                     << " is different from required version:" << required_dtc_fw_version
                     << " BAIL OUT";
  }
  else {
    _dtc_i->fPcieAddr       = pcie_addr;
    _dtc_i->fEnabled        = dtc_enabled;

    _dtc_i->fDtcID          = _odb_i->GetDtcID         (H_Dtc);
    _dtc_i->fMacAddrByte    = _odb_i->GetDtcMacAddrByte(H_Dtc);
    _dtc_i->fEmulateCfo     = _odb_i->GetDtcEmulatesCfo(H_Dtc);
        
    _dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(H_Dtc);
    _dtc_i->fEventMode      = _odb_i->GetEventMode        (H_RunConf);
    _dtc_i->fRocReadoutMode = _odb_i->GetRocReadoutMode   (H_RunConf);
    _dtc_i->fJAMode         = _odb_i->GetJAMode           (H_Dtc);

    TLOG(TLVL_DEBUG) << "subsystem:"         << subsystem
                     << std::format(" dtc_fw_version:0x{:08x}",dtc_fw_version)
                     << " _readout_mode:"    << std::dec << _dtc_i->fRocReadoutMode
                     << " roc_readout_mode:" << _dtc_i->fRocReadoutMode
                     << " sample_edge_mode:" << _dtc_i->fSampleEdgeMode
                     << " event_mode:"       << _dtc_i->fEventMode
                     << " emulate_cfo:"      << _dtc_i->fEmulateCfo;
//-----------------------------------------------------------------------------
// loop over links, redefine the enabled link mask (also in ODB)
// also store in ODB IDs of the ROCs
//-----------------------------------------------------------------------------
    int mask = 0;
    for (int i=0; i<6; i++) {
      int link_enabled = _odb_i->GetLinkEnabled(H_Dtc,i);
      TLOG(TLVL_DEBUG) << "link:" << i << " link_enabled:" << link_enabled;
      if (link_enabled) {
        mask |= (1 << 4*i);
//-----------------------------------------------------------------------------
// not tested for CRV
//-----------------------------------------------------------------------------
//         std::string roc_id     ("READ_ERROR");
//         std::string design_info("READ_ERROR");
//         std::string git_commit ("READ_ERROR");
        
//         try {
//           roc_id      = _dtc_i->GetRocID         (i);
//           TLOG(TLVL_DEBUG) << "roc_id:" << roc_id;
//           design_info = _dtc_i->GetRocDesignInfo (i);
//           TLOG(TLVL_DEBUG) << "design_info:" << design_info;
//           git_commit  = _dtc_i->GetRocFwGitCommit(i);
//           TLOG(TLVL_DEBUG) << "git_commit:" << git_commit;
//         }
//         catch(...) {
//           TLOG(TLVL_ERROR) << "cant read link:" << i << " ROC info";
//         }
// //-----------------------------------------------------------------------------
// // 'h_link' points to a subsystem-specific place
// //-----------------------------------------------------------------------------
//         char key[10];
//         sprintf(key,"Link%d",i);
//         HNDLE h_link = _odb_i->GetHandle(H_Dtc,key);
//         _odb_i->SetRocID         (h_link,roc_id     );
//         _odb_i->SetRocDesignInfo (h_link,design_info);
//         _odb_i->SetRocFwGitCommit(h_link,git_commit );
      }
    }
//-----------------------------------------------------------------------------
// set link mask, also update link mask in ODB - that is not used, but is convenient
//-----------------------------------------------------------------------------
    _dtc_i->fLinkMask      = mask;
    _odb_i->SetLinkMask(H_Dtc,mask);
//-----------------------------------------------------------------------------
// monitoring
//-----------------------------------------------------------------------------
    _monitoringLevel     = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/DTC"  );
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
      std::string m = std::format("cannot open DTC{} hotlink in ODB",_dtc_i->PcieAddr());
      cm_msg(MERROR, __func__,m.data());
    }
  }

  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  _logfile             = std::format("{}/crvdtc.log",data_dir);

  TLOG(TLVL_DEBUG) << "-- END _logfile:" << _logfile;
}

//-----------------------------------------------------------------------------
TEqCrvDtc::~TEqCrvDtc() {
}

//-----------------------------------------------------------------------------
// can afford printing every time - once per run
//-----------------------------------------------------------------------------
int TEqCrvDtc::BeginRun(HNDLE H_RunConf) {
  int rc(0);
    
  TLOG(TLVL_DEBUG) << "-- START: DTC" << _dtc_i->PcieAddr() << ":" << _dtc_i;

  int   event_mode        = _odb_i->GetEventMode     (H_RunConf);
  int   roc_readout_mode  = _odb_i->GetRocReadoutMode(H_RunConf);

  TLOG(TLVL_DEBUG) << "event mode:"        << event_mode
                   << " roc_readout_mode:" << roc_readout_mode;
  if (_dtc_i) {
    _dtc_i->fEventMode      = event_mode;
    _dtc_i->fRocReadoutMode = roc_readout_mode;
    _dtc_i->fLinkMask       = _odb_i->GetLinkMask         (_h_dtc);
    _dtc_i->fJAMode         = _odb_i->GetJAMode           (_h_dtc);
    _dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(_h_dtc);
//-----------------------------------------------------------------------------
// HardReset erases the DTC link mask, restore it
// also, release all buffers from the previous read - this is the initialization
// InitReadout performs some soft resets, ok for now
//-----------------------------------------------------------------------------
    rc = _dtc_i->InitReadout();
  }
  
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;
  return rc;
}

//-----------------------------------------------------------------------------
TMFeResult TEqCrvDtc::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::InitVarNames() {
  return 0;
}


//-----------------------------------------------------------------------------
int TEqCrvDtc::HandlePeriodic() {
  return 0;
}
