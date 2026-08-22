///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include "odbxx.h"

#include "utils/OdbInterface.hh"
#include "node_frontend/TEqCalDtc.hh"
#include "utils/TEquipmentManager.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCalDtc"
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
  
};

//-----------------------------------------------------------------------------
TEqCalDtc::TEqCalDtc(const char* Name, const char* Title) : TMu2eEqBase(Name,Title,TMu2eEqBase::kTracker) {
}

//-----------------------------------------------------------------------------
TEqCalDtc::~TEqCalDtc() {
}

//-----------------------------------------------------------------------------
TEqCalDtc::TEqCalDtc(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Dtc): TMu2eEqBase(Name,Title,TMu2eEqBase::kTracker)
{
  TLOG(TLVL_DEBUG) << "-- START: H_RunConf:" << H_RunConf << " H_Dtc:" << H_Dtc;
  
  _handle              = H_Dtc;

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
  if (subsystem == "CAL") {
    _dtc_i = (mu2edaq::DtcInterfaceBase*) DtcInterfaceCal::Instance(pcie_addr,link_mask,skip_dtc_init);
    _dtc_i->fSubsystem = mu2edaq::kCAL;
  }
  else {
    TLOG(TLVL_ERROR) << std::format("cant be true");
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
    HNDLE h_daq             = _odb_i->GetDaqConfigHandle(H_RunConf);
    _dtc_i->fSampleEdgeMode = _odb_i->GetInteger(h_daq,"ForceCfoSampleEdgeSelect");
    
    _dtc_i->fEventMode      = _odb_i->GetEventMode        (H_RunConf);
    _dtc_i->fRocReadoutMode = _odb_i->GetRocReadoutMode   (H_RunConf);
    _dtc_i->fJAMode         = _odb_i->GetJAMode           (H_Dtc);

    TLOG(TLVL_DEBUG) << "subsystem:"         << subsystem
                     << std::format(" fw_version:0x{:08x}",fw_version)
                     << " _readout_mode:"    << std::dec << _dtc_i->fRocReadoutMode
                     << " roc_readout_mode:" << _dtc_i->fRocReadoutMode
                     << " sample_edge_mode:" << _dtc_i->fSampleEdgeMode
                     << " event_mode:"       << _dtc_i->fEventMode
                     << " emulate_cfo:"      << _dtc_i->fEmulateCfo
      // << std::format(" roc_lane_mask:0x{:04x}",_dtc_i->fRocLaneMask)
      ;
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
int TEqCalDtc::BeginRun(int RunNumber) {
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
int TEqCalDtc::EndRun(int RunNumber) {
  int rc(0);

    
  TLOG(TLVL_DEBUG) << "-- START: DTC" << _dtc_i->PcieAddr() << ":" << _dtc_i;
  TLOG(TLVL_DEBUG) << "-- END ... do nothing ... rc:" << rc;

 
  TLOG(TLVL_DEBUG) << "-- END rc:" << rc;
  return rc;
}

//-----------------------------------------------------------------------------
TMFeResult TEqCalDtc::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int TEqCalDtc::InitVarNames() {

  TLOG(TLVL_DEBUG) << "-- START HostLabel:" << HostLabel();
  
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
// is called only if ::MonitoringLevel() > 0
//-----------------------------------------------------------------------------
int TEqCalDtc::HandlePeriodic() {
  int rc(0);
  TLOG(TLVL_DEBUG+1) << std::format("-- START: host:{} DTC:{}",HostLabel(),_dtc_i->PcieAddr());
  TLOG(TLVL_DEBUG+1) << "-- END";
  return rc;
}


//----------------------------------------------------------------------------------------
// it is helpful to redefine the monitoring level during the run - teh function is called rare enough
//----------------------------------------------------------------------------------------
int TEqCalDtc::MonitoringLevel() {
  int level = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/DTC");
  return level;
}

//------------------------------------------------------------------------------
// set ODB status if a link 'Link'
//-----------------------------------------------------------------------------
void TEqCalDtc::SetLinkStatus(int Link, int Status) {
  std::string link_status_path = std::format("Link{}/Status",Link);
  _odb_i->SetInteger(_handle,link_status_path.data(),Status);
}
