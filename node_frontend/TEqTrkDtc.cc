///////////////////////////////////////////////////////////////////////////////
#include "odbxx.h"

#include "utils/OdbInterface.hh"
#include "node_frontend/TEqTrkDtc.hh"
#include "node_frontend/TEquipmentManager.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTrkDtc"
namespace {
//                                            Temp, VCCINT, VCCAUX, VCBRAM
  std::initializer_list<int> DtcRegHist = { 0x9010, 0x9014, 0x9018, 0x901c};
  

  std::initializer_list<int>  DtcRegisters = {
    0x9004, 0x9100, 0x9114, 0x9140, 0x9144,
    0x9158, 0x9188, 0x91a8, 0x91ac, 0x91bc, 
    0x91c0, 0x91c4, 0x91f4, 0x91f8, 0x93e0
  };
  
  // some ROC registers are listed in decimal format, and some - in hex
  std::initializer_list<int> RocRegisters = {
       0,   18,    8,   15,   16,    7,      6,    4,
      23,   24,   25,   26,   11,   12,     65,   65,   17,   28,
      29,   30,   31,   32,   33,   34,      9,   10,   35,   36,
      13,
      37,   38,   38,   40,   41,   42,     43,   44,   45,   46,
      48,   49,   51,   52,   54,   55,     57,   58,
      72,   73,   74,   75,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95
  };
  
};

//-----------------------------------------------------------------------------
TEqTrkDtc::TEqTrkDtc(const char* EqName) : TMu2eEqBase(EqName) {
}

//-----------------------------------------------------------------------------
TEqTrkDtc::~TEqTrkDtc() {
}

//-----------------------------------------------------------------------------
TEqTrkDtc::TEqTrkDtc(const char* EqName, HNDLE H_RunConf, HNDLE H_Dtc)  : TMu2eEqBase(EqName) {

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
  _dtc_i->fSubsystem = mu2edaq::kTracker;
//-----------------------------------------------------------------------------
// start from checking the DTC FW verion and comparing it to the required one -
// defined in ODB
//-----------------------------------------------------------------------------
  uint32_t required_fw_version = _odb_i->GetDtcFwVersion(H_RunConf,subsystem.data());
  uint32_t fw_version          = _dtc_i->ReadRegister(0x9004);

  if ((required_fw_version != 0) and (fw_version != required_fw_version)) {
    std::string msg = std::format("DTC{}@{} has fw version:0x{:08x} different from required version:0x{:08x}",
                                  _dtc_i->PcieAddr(),HostLabel(),fw_version,required_fw_version);
    TLOG(TLVL_ERROR) << msg;
                                        // and send an error message
    cm_msg(MERROR, __func__,msg.data());
                                    
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
    _dtc_i->fRocLaneMask    = _odb_i->GetUInt32           (H_Dtc,"RocLaneMask");

    TLOG(TLVL_DEBUG) << "subsystem:"         << subsystem
                     << std::format(" fw_version:0x{:08x}",fw_version)
                     << " _readout_mode:"    << std::dec << _dtc_i->fRocReadoutMode
                     << " roc_readout_mode:" << _dtc_i->fRocReadoutMode
                     << " sample_edge_mode:" << _dtc_i->fSampleEdgeMode
                     << " event_mode:"       << _dtc_i->fEventMode
                     << " emulate_cfo:"      << _dtc_i->fEmulateCfo
                     << " roc_lane_mask:0x"  << std::hex << _dtc_i->fRocLaneMask      ;
//-----------------------------------------------------------------------------
// loop over links, redefine the enabled link mask (also in ODB)
// also store in ODB IDs of the ROCs
//-----------------------------------------------------------------------------
    int mask = 0;
    for (int i=0; i<6; i++) {
      int link_enabled = _odb_i->GetLinkEnabled(H_Dtc,i);
      TLOG(TLVL_DEBUG) << "link:" << i << " link_enabled:" << link_enabled;
      if (link_enabled) {
        if (not _dtc_i->LinkLocked(i)) {
          TLOG(TLVL_ERROR) << std::format("{}:DTC{} link:{} enabled but not locked",HostLabel(),_dtc_i->PcieAddr(),i);
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
          TLOG(TLVL_ERROR) << std::format("{}:DTC{}: cant read link:() ROC info",HostLabel(),_dtc_i->PcieAddr(),i);
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
      }
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
      std::string m = std::format("cannot open DTC{} hotlink in ODB",_dtc_i->PcieAddr());
      cm_msg(MERROR, __func__,m.data());
    }
  }
  
  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  _logfile             = std::format("{}/trkdtc.log",data_dir);

  TLOG(TLVL_DEBUG) << "-- END";
}


//-----------------------------------------------------------------------------
// can afford printing every time - once per run
//-----------------------------------------------------------------------------
int TEqTrkDtc::BeginRun(HNDLE H_RunConf) {
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
//-----------------------------------------------------------------------------
    // 2025-01-19 PM dtc_i->Dtc()->HardReset();
        // 2025-01-19 PM dtc_i->ResetLinks(0,1);
                                        // InitReadout performs some soft resets, ok for now
    rc = _dtc_i->InitReadout();
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
  std::initializer_list<const char*> dtc_names = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};

  const std::string eq_path       = "/Equipment/"+HostLabel();
  const std::string settings_path = eq_path+"/Settings";

  midas::odb        odb_settings(settings_path);

  int pcie_addr = _dtc_i->PcieAddr();
  
  std::vector<std::string> dtc_var_names;
  for (const char* name : dtc_names) {
    std::string var_name = std::format("dtc{}#{:s}",pcie_addr,name);
    dtc_var_names.push_back(var_name);
  }

  char dirname[32];
  sprintf(dirname,"Names dtc%i",pcie_addr);
  odb_settings[dirname] = dtc_var_names;
//-----------------------------------------------------------------------------
// non-history DTC registers
//-----------------------------------------------------------------------------
  dtc_var_names.clear();
  for (const int& reg : DtcRegisters) {
    char var_name[32];
    sprintf(var_name,"0x%04x",reg);
    dtc_var_names.push_back(var_name);
  }
      
  sprintf(dirname,"DTC%i",pcie_addr);

  midas::odb   odb_dtc(eq_path+"/"+dirname);
  odb_dtc["RegName"] = dtc_var_names;

  std::vector<uint32_t> dtc_reg_data(dtc_var_names.size());
  odb_dtc["RegData"] = dtc_reg_data;
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
      
    sprintf(dirname,"Names rc%i%i",pcie_addr,ilink);
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
  
  std::string node_eq_path = "/Equipment/"+HostLabel();

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

  char buf[64];
  
  sprintf(buf,"%s/DTC%i",node_eq_path.data(),_dtc_i->PcieAddr());
      
  TLOG(TLVL_DEBUG+1) << "N(DTC registers):" << DtcRegisters.size() << " buf:" << buf;
        
  midas::odb xx = {{"RegData",{1u}}};
  xx.connect(buf);
  xx["RegData"].resize(DtcRegisters.size());
  xx["RegData"] = dtc_reg;

  TLOG(TLVL_DEBUG+1) << "--END , saved to:" << buf;
}

//-----------------------------------------------------------------------------
int TEqTrkDtc::HandlePeriodic() {

  TLOG(TLVL_DEBUG+1) << std::format("-- START: host:{} DTC:{}",HostLabel(),_dtc_i->PcieAddr());
//-----------------------------------------------------------------------------
// DTC temperature and voltages - for history 
//-----------------------------------------------------------------------------
  int pcie_addr = _dtc_i->PcieAddr();
  
  char dtc_name[16];
  sprintf(dtc_name,"dtc%i",pcie_addr);

  midas::odb o_runinfo("/Runinfo");
  int running_state          = o_runinfo["State"];
  int transition_in_progress = o_runinfo["Transition in progress"];

  //  TEquipmentManager* tem = TEquipmentManager::Instance();

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
    
    std::string node_eq_path = "/Equipment/"+HostLabel();
    midas::odb odb_dtc_tv = {{dtc_name,{1.0f, 1.0f, 1.0f, 1.0f}}};
    odb_dtc_tv.connect(node_eq_path+"/Variables");
        
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
    for (int ilink=0; ilink<6; ilink++) {
      if (_dtc_i->LinkEnabled(ilink) == 0) continue;
      if (not _dtc_i->LinkLocked(ilink)) {
        TLOG(TLVL_ERROR) << std::format("{}:DTC{} link:{} enabled but not locked",HostLabel(),_dtc_i->PcieAddr(),ilink);
        continue;
      }
          
      if (_monitorRocRegisters > 0) {
            
        std::vector<uint32_t>  roc_reg;
        roc_reg.reserve(RocRegisters.size());
            
        try {
          for (const int reg : RocRegisters) {
                // ROC registers store 16-bit words, don't know how to declare an array
                // of shorts for ODBXX, use uint32_t
            uint32_t dat = _dtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
            roc_reg.emplace_back(dat);
          }
              
          char buf[100];
          sprintf(buf,"%s/DTC%i/ROC%i",node_eq_path.data(),_dtc_i->PcieAddr(),ilink);
              
          midas::odb roc = {{"RegData",{1u}}};
          roc.connect(buf);
          roc["RegData"] = roc_reg;
        }
        catch (...) {
          TLOG(TLVL_ERROR) << "failed to read DTC:" << _dtc_i->PcieAddr() << " ROC:" << ilink << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
            // TODO
        }
      }
//-----------------------------------------------------------------------------
// SPI
//-----------------------------------------------------------------------------
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
          xx.connect(node_eq_path+"/Variables");

          int nw = trkdaq::TrkSpiDataNWords+trkdaq::TrkKeyDataNWords+trkdaq::TrkIlpDataNWords;
          xx[buf].resize(nw);
          xx[buf] = roc_spi;
              
          TLOG(TLVL_DEBUG+1) << "ROC:" << ilink
                             << " saved N(SPI+KEY+ILP) words:" << nw;
        }
        else {
          TLOG(TLVL_ERROR)   << "failed to read SPI, DTC:" << _dtc_i->PcieAddr() << " ROC:" << ilink;
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
            // TODO
        }
      }
//-----------------------------------------------------------------------------
// ROC rates
// for now, assume that the clock has been set to internal ,
// need to find the right place to set marker_clock to 0 (and may be recover in the end),
// will do it right later 
//-----------------------------------------------------------------------------
      if ((_monitorRates > 0) and (transition_in_progress == 0) and (running_state != STATE_RUNNING)) {
        TLOG(TLVL_DEBUG+1) << "MONITOR RATES link:" << ilink;
//-----------------------------------------------------------------------------
// for monitoring, want to read ALL channels.
// 1. run read command enabling the internal clock and setting the read mask to read all channels
// 2. run rates command with all channels enabled
// 3. run read command restoring the clock marker (source of the clock) and the read mask
//    which someone may rely on
//-----------------------------------------------------------------------------
        midas::odb o_read_cmd   ("/Mu2e/Commands/Tracker/DTC/control_roc_read");

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
            
        _dtc_i->ControlRoc_Read(&pread,ilink,print_level);
               
        std::vector<uint16_t> rates;
        trkdaq::ControlRoc_Rates_t* par(nullptr); // defaults are OK - read all channels
        int rc = _dtc_i->ControlRoc_Rates(ilink,&rates,print_level,par,nullptr);
//-----------------------------------------------------------------------------
// and restore the READ command mask and the clock
//-----------------------------------------------------------------------------
        pread.marker_clock    = o_read_cmd["marker_clock" ];   // restore the marker_clock mode
        for (int i=0; i<6; ++i) pread.ch_mask[i] = saved_ch_mask[i];
        _dtc_i->ControlRoc_Read(&pread,ilink,print_level);
//-----------------------------------------------------------------------------
// print diagnostics
//-----------------------------------------------------------------------------
        if (rc == 0) {
          char buf[16];
          sprintf(buf,"rr%i%i",_dtc_i->PcieAddr(),ilink);
          
          midas::odb vars(node_eq_path+"/Variables");
          vars[buf] = rates;
          
          TLOG(TLVL_DEBUG+1) << "ROC:" << ilink << " saved rates to \""
                             << node_eq_path+"/Variables[" << buf << "\"], nw:" << rates.size();
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
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "failed to read DTC:" << _dtc_i->PcieAddr() << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
    // TODO
  }

  TLOG(TLVL_DEBUG+1) << "-- END";
  return 0;
}

