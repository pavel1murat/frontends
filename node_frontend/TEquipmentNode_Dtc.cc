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
  int event_mode       = _odb_i->GetEventMode     (_h_active_run_conf);
  int roc_readout_mode = _odb_i->GetRocReadoutMode(_h_active_run_conf);
  int skip_dtc_init    = _odb_i->GetSkipDtcInit   (_h_active_run_conf);

  HNDLE h_subkey;
  KEY   subkey;

  for (int i=0; db_enum_key(hDB,_h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);
    
    TLOG(TLVL_DEBUG) << "subkey.name:" << subkey.name;
    
    if (strstr(subkey.name,"DTC") != subkey.name)           continue;
    
    int enabled          = _odb_i->GetDtcEnabled    (hDB,h_subkey);
    int pcie_addr        = _odb_i->GetDtcPcieAddress(h_subkey);
    int link_mask        = _odb_i->GetDtcLinkMask   (h_subkey);
    
    TLOG(TLVL_DEBUG) << "link_mask:0x" <<std::hex << link_mask << std::endl; 
    
    if (enabled) {
      // for now, disable re-initialization
      mu2edaq::DtcInterface* dtc_i = nullptr;
      if(_odb_i->GetIsCrv(h_subkey)) {
        bool initRocs = true;
        bool initFebs = true;
        dtc_i = crvdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init,initRocs,initFebs);
        TLOG(TLVL_DEBUG) << "crvdaq::DtcInterface";
        // CRV specific
        //dynamic_cast<crvdaq::DtcInterface*>(dtc_i)->SetInit(initRocs, initFebs);
        //if(initRocs) {
        //    dynamic_cast<crvdaq::DtcInterface*>(dtc_i)->InitRocs();
        //}
        //dtc_i = mu2edaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
        //dtc_i->fIsCrv = 1;
        // TODO CRV specific inits from ODB

      } else {
        dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
      }

      dtc_i->fLinkMask       = link_mask;
      dtc_i->fPcieAddr       = pcie_addr;
      dtc_i->fEnabled        = enabled;
        
      dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
      dtc_i->fEmulateCfo     = _odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);

      
      dtc_i->fEventMode      = event_mode;
      dtc_i->fRocReadoutMode = roc_readout_mode;
        
      TLOG(TLVL_DEBUG) << "is_crv:"            << dtc_i->fIsCrv
                       << " roc_readout_mode:" << dtc_i->fRocReadoutMode
                       << " sample_edge_mode:" << dtc_i->fSampleEdgeMode
                       << " event_mode:"       << dtc_i->fEventMode
                       << " emulate_cfo:"      << dtc_i->fEmulateCfo;
      
      fDtc_i[pcie_addr]   = dtc_i;
    }
  }

  InitDtcVarNames();

  return TMFeOk();
}

//-----------------------------------------------------------------------------
// 2 DTCs and their ROCs
//-----------------------------------------------------------------------------
void TEquipmentNode::InitDtcVarNames() {
  char dirname[256], var_name[128];
  
  midas::odb::set_debug(true);

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
    for (const char* name : DtcRegHistNames) {
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
    //if(!fDtc_i[idtc]->IsCrv()) { // no CRV
     // for (int ilink=0; ilink<6; ilink++) { 
    // 
    //    std::vector<std::string> roc_var_names;
    //    for (int k=0; k<trkdaq::TrkSpiDataNWords; k++) {
    //      sprintf(var_name,"rc%i%i#%s",idtc,ilink,trkdaq::DtcInterface::SpiVarName(k));
    //      roc_var_names.push_back(var_name);
    //    }
    //    sprintf(dirname,"Names rc%i%i",idtc,ilink);
    //    if (not midas::odb::exists(settings_path+"/"+dirname)) {
    //      // midas::odb o = {dirname,{"a"}};
    //      // o.connect(settings_path);
    //      odb_settings[dirname] = roc_var_names;
    //    }
//-----------------------------------------------------------------------------
// non-history ROC registers - counters and such - just to be looked at
//-----------------------------------------------------------------------------
    //    roc_var_names.clear();
    //    for(const int& reg : trkdaq::RocRegisters) {
    //      sprintf(var_name,"reg_%03i",reg);
    //      roc_var_names.push_back(var_name);
    //    }

    //    char roc_subdir[128];

    //    sprintf(roc_subdir,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
    //    midas::odb odb_roc = {{"RegName",{"a","b"}},{"RegData",{1u,2u}}};
    //    odb_roc.connect(roc_subdir);
    //    // odb_roc["RegName"].resize(roc_var_names.size());;
    //    odb_roc["RegName"] = roc_var_names;
    //  
    //    std::vector<uint16_t> roc_reg_data(roc_var_names.size());
    //    // odb_roc["RegData"].resize(roc_reg_data.size());
    //    odb_roc["RegData"] = roc_reg_data;
    //  }
    //} else { // CRV
      mu2edaq::DtcInterface* dtc_i = fDtc_i[idtc];
      for (int ilink=0; ilink<6; ilink++) {
        // skip disabled links
        mu2edaq::DtcInterface* dtc_i = fDtc_i[idtc];
        if (dtc_i->LinkEnabled(ilink)) {
            //---------------------------------
            // history ROC registers
            //---------------------------------
            sprintf(dirname,"Names rc%i%i",idtc,ilink);
            if (not midas::odb::exists(settings_path+"/"+dirname)) {
              odb_settings[dirname] = fDtc_i[idtc]->GetRocRegistersNames(true);
            }
            //---------------------------------
            // CRV specific FEB/port variables
            //---------------------------------
            if(fDtc_i[idtc]->IsCrv()) {
                auto crvdtc_i = dynamic_cast<crvdaq::DtcInterface*>(dtc_i);
                auto link = DTCLib::DTC_Link_ID(ilink);
                uint32_t activePorts = crvdtc_i->GetRocActivePorts(link);
                //uint32_t activePorts = crvdtc_i->GetRocActivePorts(ilink);
                TLOG(TLVL_DEBUG) << "activePorts: 0x" << std::hex << activePorts;
                for(int port=1; port <=24; port++) {
                    if (activePorts & (1 << (port-1))) {
                        sprintf(dirname,"Names rc%i%ip%02i",idtc,ilink,port);
                        if (not midas::odb::exists(settings_path+"/"+dirname)) {
                            odb_settings[dirname] = crvdtc_i->GetRocPortRegNames();
                        }
                    }
                }
            }
            //---------------------------------
            // non-history ROC registers 
            //---------------------------------

            char roc_subdir[128];

            sprintf(roc_subdir,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
            midas::odb odb_roc = {{"RegName",{"a","b"}},{"RegData",{0u,0u}}};
            odb_roc.connect(roc_subdir);
            odb_roc["RegName"] = fDtc_i[idtc]->GetRocRegistersNames(false);
            odb_roc["RegData"].resize(odb_roc["RegName"].size());
        } else {
            // if the ROC doesn't exist or is disabled, do we want to remove the variables to avoid confusion?
        }
      } 
    //}
  }

  midas::odb::set_debug(false);
};

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
          dtc_i->fDtc->GetDevice()->read_register(reg,100,&val); 
          if      ( reg == 0x9010) fval = (val/4096.)*503.975 - 273.15;   // temperature
          else                     fval = (val/4095.)*3.;                 // voltage
          
          dtc_tv.emplace_back(fval);
        }

        char buf[100];
        sprintf(buf,"dtc%i",idtc);
        
        midas::odb odb_dtc_tv = {{buf,{1.0f, 1.0f, 1.0f, 1.0f}}};
        odb_dtc_tv.connect(node_path+"/Variables");
        
        odb_dtc_tv[buf] = dtc_tv;
//-----------------------------------------------------------------------------
// non-history registers : dtr - "DTcRegisters"
//-----------------------------------------------------------------------------
        std::vector<uint32_t>  dtc_reg;
        dtc_reg.reserve(DtcRegisters.size());
        
        for (const int reg : DtcRegisters) {
          uint32_t dat;
          dtc_i->fDtc->GetDevice()->read_register(reg,100,&dat); 
          dtc_reg.emplace_back(dat);
        }

        sprintf(buf,"%s/DTC%i",node_path.data(),idtc);
      
        TLOG(TLVL_DEBUG+1) << "N(DTC registers):" << DtcRegisters.size()
                           << " buf:" << buf;
        
        midas::odb xx = {{"RegData",{1u}}};
        xx.connect(buf);
        xx["RegData"].resize(DtcRegisters.size());
        xx["RegData"] = dtc_reg;

        TLOG(TLVL_DEBUG+1) << "saved to:" << buf;
//-----------------------------------------------------------------------------
// for each enabled DTC, loop over its ROCs and read ROC registers
// this part can depend on on the type of the ROC
// do it for the tracker
// don't use 'link' - ROOT doesn't like 'link' for a variable name
//-----------------------------------------------------------------------------
        //if(!dtc_i->IsCrv()) {
        //  auto trkdtc_i = dynamic_cast<trkdaq::DtcInterface*>(dtc_i);
        //  for (int ilink=0; ilink<6; ilink++) {
        //    if (trkdtc_i->LinkEnabled(ilink)) {
        //
        //      if (_monitorRocRegisters) {
        //
        //        std::vector<uint32_t>  roc_reg;
        //        roc_reg.reserve(trkdaq::RocRegisters.size());
        //
        //        try {
        //          for (const int reg : trkdaq::RocRegisters) {
        //            // ROC registers store 16-bit words, don't know how to declare an array
        //            // of shorts for ODBXX, use uint32_t
        //            uint32_t dat = trkdtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
        //            roc_reg.emplace_back(dat);
        //          }
        //      
        //          char buf[100];
        //          sprintf(buf,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
        //
        //          midas::odb roc = {{"RegData",{1u}}};
        //          roc.connect(buf);
        //          roc["RegData"] = roc_reg;
        //        }
        //        catch (...) {
        //          TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " ROC:" << ilink << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
        //        // TODO
        //        }
        //      }

        //      if (_monitorRocSPI) {
        //        TLOG(TLVL_DEBUG+1) << "saving ROC:" << ilink << " SPI data";
        //      
        //        try { 
        //          std::vector<uint16_t> spi_raw_data;
        //          struct trkdaq::TrkSpiData_t   spi;
        //          trkdtc_i->ReadSpiData   (ilink,spi_raw_data,0);
        //          trkdtc_i->ConvertSpiData(spi_raw_data,&spi,0);
        //      
        //          std::vector<float> roc_spi;
        //      
        //          for (int iw=0; iw<trkdaq::TrkSpiDataNWords; iw++) {
        //            roc_spi.emplace_back(spi.Data(iw));
        //          }
        //      
        //          char buf[100];
        //          sprintf(buf,"rc%i%i",idtc,ilink);
        //      
        //          midas::odb xx = {{buf,{1.0f}}};
        //          xx.connect(node_path+"/Variables");
        //        
        //          xx[buf].resize(trkdaq::TrkSpiDataNWords);
        //          xx[buf] = roc_spi;
        //        
        //          TLOG(TLVL_DEBUG+1) << "ROC:" << ilink
        //                           << " saved N(SPI) words:" << trkdaq::TrkSpiDataNWords;
        //        }
        //        catch(...) {
        //          TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " ROC:" << ilink << " SPI";
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
        //        // TODO
        //        }
        //      }
        //    }
        //  }
        //} else { // CRV ROC
          //if (_monitorRocRegisters) {
            for (int ilink=0; ilink<6; ilink++) {
              char buf[100];

              if (dtc_i->LinkEnabled(ilink)) {
                if (_monitorRocSPI|_monitorRoc) {
                  sprintf(buf,"rc%i%i",idtc,ilink);
              
                  midas::odb xx = {{buf,{1.0f}}};
                  xx.connect(node_path+"/Variables");
                  auto roc_data = dtc_i->GetConvertedRocRegisters(ilink, true);
                  xx[buf].resize(roc_data.size()); // we could do this ones when we create it?
                  xx[buf] = roc_data;


                  if(fDtc_i[idtc]->IsCrv()) {
                    auto crvdtc_i = dynamic_cast<crvdaq::DtcInterface*>(dtc_i);
                    auto link = DTCLib::DTC_Link_ID(ilink);
                    uint32_t activePorts = crvdtc_i->GetRocActivePorts(link);
                    for(int port=1; port <=24; port++) {
                        if (activePorts & (1 << (port-1))) {
                            sprintf(buf,"Names rc%i%ip%02i",idtc,ilink,port);
                            midas::odb xxx = {{buf,{1.0f}}};
                            xxx.connect(node_path+"/Variables");
                            auto roc_pool_data = crvdtc_i->GetRocPortRegValues(ilink, port);
                            xxx[buf].resize(roc_pool_data.size());
                            xxx[buf] = roc_pool_data;
                        }
                    }
                  }
                }
                if (_monitorRocRegisters) {
                  sprintf(buf,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);

                  midas::odb roc = {{"RegData",{1u}}};
                  roc.connect(buf);
                  roc["RegData"] = dtc_i->GetRocRegisters(ilink, false);
                }
              }
            }
            // todo, add CRV FEBs?
          //}
        //}
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
