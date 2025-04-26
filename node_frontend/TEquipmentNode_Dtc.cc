//////////////////////////////////////////////////////////////////////////////
// equipment name is the short node name, i.e. 'mu2edaq22'
//////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode_Dtc"

//-----------------------------------------------------------------------------
// back to DTC: two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::InitDtc() {
  
  int skip_dtc_init    = _odb_i->GetSkipDtcInit   (_h_active_run_conf);

  HNDLE h_subkey;
  KEY   subkey;

  TLOG(TLVL_DEBUG) << "--- START";

  for (int i=0; db_enum_key(hDB,_h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);
    
    TLOG(TLVL_DEBUG) << "subkey.name:" << subkey.name;
    
    if (strstr(subkey.name,"DTC") != subkey.name)           continue;
    
    int dtc_enabled      = _odb_i->GetEnabled       (h_subkey);
    TLOG(TLVL_DEBUG) << "enabled:" << dtc_enabled;

    int pcie_addr        = _odb_i->GetDtcPcieAddress(h_subkey);
    int link_mask        = _odb_i->GetLinkMask      (h_subkey);
    TLOG(TLVL_DEBUG) << "link_mask:0x" <<std::hex << link_mask
                     << " DTC pcie_addr:" << pcie_addr
                     << " dtc_enabled:" << dtc_enabled;

    _h_dtc[pcie_addr]    = h_subkey;
    
    if (dtc_enabled) {

      // for now, disable re-initialization
      mu2edaq::DtcInterface* dtc_i = nullptr;
      if(_odb_i->GetIsCrv(h_subkey)) {
        // TODO
        // dtc_i = crvdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
        dtc_i = mu2edaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
        dtc_i->fIsCrv = 1;
      } else {
        dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
      }

      dtc_i->fLinkMask       = link_mask;
      dtc_i->fPcieAddr       = pcie_addr;
      dtc_i->fEnabled        = dtc_enabled;

      dtc_i->fDtcID          = _odb_i->GetDtcID         (h_subkey);
      dtc_i->fMacAddrByte    = _odb_i->GetDtcMacAddrByte(h_subkey);
      dtc_i->fEmulateCfo     = _odb_i->GetDtcEmulatesCfo(h_subkey);
        
      dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(h_subkey);
      dtc_i->fEventMode      = _odb_i->GetEventMode        (_h_active_run_conf);
      dtc_i->fRocReadoutMode = _odb_i->GetRocReadoutMode   (_h_active_run_conf);
      dtc_i->fJAMode         = _odb_i->GetJAMode           (h_subkey);

      TLOG(TLVL_DEBUG) << "is_crv:"            << dtc_i->fIsCrv
                       << " roc_readout_mode:" << dtc_i->fRocReadoutMode
                       << " sample_edge_mode:" << dtc_i->fSampleEdgeMode
                       << " event_mode:"       << dtc_i->fEventMode
                       << " emulate_cfo:"      << dtc_i->fEmulateCfo;
//-----------------------------------------------------------------------------
// loop over links and store in ODB IDs of the ROCs
// this place is tracker-specific
//-----------------------------------------------------------------------------
      int mask = 0;
      for (int i=0; i<6; i++) {
        // int link_enabled = dtc_i->LinkEnabled(i);
        int link_enabled = _odb_i->GetLinkEnabled(h_subkey,i);
        TLOG(TLVL_DEBUG) << "link:" << i << " link_enabled:" << link_enabled;
        if (link_enabled) {
          mask |= (1 << 4*i);
            
          std::string roc_id     ("READ_ERROR");
          std::string design_info("READ_ERROR");
          std::string git_commit ("READ_ERROR");
          try {
            roc_id      = dtc_i->GetRocID         (i);
            TLOG(TLVL_DEBUG) << "roc_id:" << roc_id;
            dtc_i->GetRocDesignInfo (i);
            TLOG(TLVL_DEBUG) << "design_info:" << design_info;
            git_commit  = "READ_DISABLED" ; // dtc_i->GetRocFwGitCommit(i);
            TLOG(TLVL_DEBUG) << "git_commit:" << git_commit;
          }
          catch(...) {
            TLOG(TLVL_ERROR) << "cant read link:" << i << " ROC info";
          }
//-----------------------------------------------------------------------------
// 'h_link' points to a subsystem-specific place
//-----------------------------------------------------------------------------
          char key[10];
          sprintf(key,"Link%d",i);
          HNDLE h_link = _odb_i->GetHandle(h_subkey,key);
          _odb_i->SetRocID         (h_link,roc_id     );
          _odb_i->SetRocDesignInfo (h_link,design_info);
          _odb_i->SetRocFwGitCommit(h_link,git_commit );
        }
      }
//-----------------------------------------------------------------------------
// set link mask, also update link mask in ODB - that is not used, but is convenient
//-----------------------------------------------------------------------------
      dtc_i->fLinkMask       = mask;
      _odb_i->SetLinkMask(h_subkey,mask);
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
// don't reset ROCs on enabled links
//-----------------------------------------------------------------------------
      // dtc_i->ResetLinks();
      fDtc_i[pcie_addr]   = dtc_i;
    }
  }

  InitDtcVarNames();
  
  TLOG(TLVL_DEBUG) << "--- END: DTC0:" <<  fDtc_i[0] << " DTC1:" <<  fDtc_i[1];

  return TMFeOk();
}

//-----------------------------------------------------------------------------
// 2 DTCs and their ROCs
//-----------------------------------------------------------------------------
void TEquipmentNode::InitDtcVarNames() {
  char dirname[256], var_name[128];
  
  TLOG(TLVL_DEBUG) << " --- START";
// midas::odb::set_debug(true);

  // SC TODO: I think we should define these together with the registers? 
  std::initializer_list<const char*> dtc_names = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};

  const std::string node_path     = "/Equipment/"+TMFeEquipment::fEqName;
  //midas::odb odb_node(node_path);
  
  const std::string settings_path = node_path+"/Settings";
  midas::odb odb_settings(node_path+"/Settings");
//-----------------------------------------------------------------------------
// DTCs and ROCs
//-----------------------------------------------------------------------------
  for (int idtc=0; idtc<2; idtc++) {
    // SC: should we not create the variables if the DTC doesn't exist?
    if(fDtc_i[idtc] == nullptr) continue;

    std::vector<std::string> dtc_var_names;
    for (const char* name : dtc_names) {
      sprintf(var_name,"dtc%i#%s",idtc,name);
      dtc_var_names.push_back(name);
    }
       
    sprintf(dirname,"Names dtc%i",idtc);
    // if (not midas::odb::exists(node_path+"/Settings/"+dirname)) {
    odb_settings[dirname] = dtc_var_names;
    // }
//-----------------------------------------------------------------------------
// non-history DTC registers
//-----------------------------------------------------------------------------
    dtc_var_names.clear();
    for (const int& reg : DtcRegisters) {
      // sprintf(var_name,"dtc%i#0x%04x",idtc,reg);
      sprintf(var_name,"0x%04x",reg);
      dtc_var_names.push_back(var_name);
    }
      
    sprintf(dirname,"DTC%i",idtc);

    midas::odb   odb_dtc(node_path+"/"+dirname);
    odb_dtc["RegName"] = dtc_var_names;

    std::vector<uint32_t> dtc_reg_data(dtc_var_names.size());
    odb_dtc["RegData"] = dtc_reg_data;
//-----------------------------------------------------------------------------
// loop over the ROCs and create names for each of them
//-----------------------------------------------------------------------------
    if(!fDtc_i[idtc]->IsCrv()) { // no CRV
      for (int ilink=0; ilink<6; ilink++) { 
     
        std::vector<std::string> roc_var_names;
        for (int k=0; k<trkdaq::TrkSpiDataNWords; k++) {
          sprintf(var_name,"rc%i%i#%s",idtc,ilink,trkdaq::DtcInterface::SpiVarName(k));
          roc_var_names.push_back(var_name);
        }
      
        sprintf(dirname,"Names rc%i%i",idtc,ilink);
        if (not midas::odb::exists(settings_path+"/"+dirname)) {
          // midas::odb o = {dirname,{"a"}};
          // o.connect(settings_path);
          odb_settings[dirname] = roc_var_names;
        }
//-----------------------------------------------------------------------------
// non-history ROC registers - counters and such - just to be looked at
//-----------------------------------------------------------------------------
        roc_var_names.clear();
        for(const int& reg : RocRegisters) {
          sprintf(var_name,"reg_%03i",reg);
          roc_var_names.push_back(var_name);
        }

        char roc_subdir[128];

        sprintf(roc_subdir,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
        midas::odb odb_roc = {{"RegName",{"a","b"}},{"RegData",{1u,2u}}};
        odb_roc.connect(roc_subdir);
        // odb_roc["RegName"].resize(roc_var_names.size());;
        odb_roc["RegName"] = roc_var_names;
      
        std::vector<uint16_t> roc_reg_data(roc_var_names.size());
        // odb_roc["RegData"].resize(roc_reg_data.size());
        odb_roc["RegData"] = roc_reg_data;
      }
    } else { // CRV
       
    }
  }

  midas::odb::set_debug(false);

  TLOG(TLVL_DEBUG) << " --- END";
};


//-----------------------------------------------------------------------------
void TEquipmentNode::ReadNonHistDtcRegisters(mu2edaq::DtcInterface* Dtc_i) {
  
  std::string node_path = "/Equipment/"+TMFeEquipment::fEqName;

  std::vector<uint32_t>  dtc_reg;
  dtc_reg.reserve(DtcRegisters.size());
        
  for (const int reg : DtcRegisters) {
    uint32_t dat(0);
    try {
      Dtc_i->fDtc->GetDevice()->read_register(reg,100,&dat);
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "failed to read register:" << reg;
      dat = 0xFFFFFFFF;
    }
    dtc_reg.emplace_back(dat);
  }

  char buf[64];
  
  sprintf(buf,"%s/DTC%i",node_path.data(),Dtc_i->PcieAddr());
      
  TLOG(TLVL_DEBUG+1) << "N(DTC registers):" << DtcRegisters.size() << " buf:" << buf;
        
  midas::odb xx = {{"RegData",{1u}}};
  xx.connect(buf);
  xx["RegData"].resize(DtcRegisters.size());
  xx["RegData"] = dtc_reg;

  TLOG(TLVL_DEBUG+1) << "saved to:" << buf;
}


//-----------------------------------------------------------------------------
void TEquipmentNode::ReadDtcMetrics() {
  char   text[200];
  
  //  double t  = TMFE::GetTime();
  // midas::odb::set_debug(true);

  std::string node_path = "/Equipment/"+TMFeEquipment::fEqName;
  
  for (int idtc=0; idtc<2; idtc++) {
    mu2edaq::DtcInterface* dtc_i = fDtc_i[idtc];
    if (dtc_i) {
//-----------------------------------------------------------------------------
// DTC temperature and voltages - for history 
//-----------------------------------------------------------------------------
      sprintf(text,"dtc%i",idtc);

      try {
        std::vector<float> dtc_tv;
        for (const int reg : DtcRegHist) {
          uint32_t val;
          float    fval(-1.);
          try { 
            dtc_i->fDtc->GetDevice()->read_register(reg,100,&val); 
            if      ( reg == 0x9010) fval = (val/4096.)*503.975 - 273.15;   // temperature
            else                     fval = (val/4095.)*3.;                 // voltage
          }
          catch(...) {
            TLOG(TLVL_ERROR) << "failed to read register:" << reg;
            fval = -1;
          }
          
          dtc_tv.emplace_back(fval);
        }

        char buf[100];
        sprintf(buf,"dtc%i",idtc);
        
        midas::odb odb_dtc_tv = {{buf,{1.0f, 1.0f, 1.0f, 1.0f}}};
        odb_dtc_tv.connect(node_path+"/Variables");
        
        odb_dtc_tv[buf] = dtc_tv;
//-----------------------------------------------------------------------------
// non-history registers : 'dtr' = "DTcRegisters"
//-----------------------------------------------------------------------------
        ReadNonHistDtcRegisters(dtc_i);
//-----------------------------------------------------------------------------
// for each enabled DTC, loop over its ROCs and read ROC registers
// this part can depend on on the type of the ROC
// do it for the tracker
// don't use 'link' - ROOT doesn't like 'link' for a variable name
//-----------------------------------------------------------------------------
        if(!dtc_i->IsCrv()) {
          auto trkdtc_i = dynamic_cast<trkdaq::DtcInterface*>(dtc_i);
          for (int ilink=0; ilink<6; ilink++) {
            if (trkdtc_i->LinkEnabled(ilink)) {

              if (_monitorRocRegisters) {

                std::vector<uint32_t>  roc_reg;
                roc_reg.reserve(RocRegisters.size());

                try {
                  for (const int reg : RocRegisters) {
                    // ROC registers store 16-bit words, don't know how to declare an array
                    // of shorts for ODBXX, use uint32_t
                    uint32_t dat = trkdtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
                    roc_reg.emplace_back(dat);
                  }
              
                  char buf[100];
                  sprintf(buf,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);

                  midas::odb roc = {{"RegData",{1u}}};
                  roc.connect(buf);
                  roc["RegData"] = roc_reg;
                }
                catch (...) {
                  TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " ROC:" << ilink << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
                // TODO
                }
              }

              if (_monitorRocSPI) {
                TLOG(TLVL_DEBUG+1) << "saving ROC:" << ilink << " SPI data";
              
                struct trkdaq::TrkSpiData_t   spi;
                int rc = trkdtc_i->ControlRoc_ReadSpi_1(&spi,ilink,0);
                if (rc == 0) {
              
                  std::vector<float> roc_spi;
                  for (int iw=0; iw<trkdaq::TrkSpiDataNWords; iw++) {
                    roc_spi.emplace_back(spi.Data(iw));
                  }
              
                  char buf[100];
                  sprintf(buf,"rc%i%i",idtc,ilink);
              
                  midas::odb xx = {{buf,{1.0f}}};
                  xx.connect(node_path+"/Variables");
                
                  xx[buf].resize(trkdaq::TrkSpiDataNWords);
                  xx[buf] = roc_spi;
                
                  TLOG(TLVL_DEBUG+1) << "ROC:" << ilink
                                   << " saved N(SPI) words:" << trkdaq::TrkSpiDataNWords;
                }
                else {
                  TLOG(TLVL_ERROR) << "failed to read SPI, DTC:" << idtc << " ROC:" << ilink;
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
                // TODO
                }
              }
            }
          }
        } else { // CRV ROC

        }
      }
      catch (...) {
        TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
        // TODO
      }
      
    }
  }
}
