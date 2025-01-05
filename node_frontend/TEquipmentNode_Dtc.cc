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
      bool skip_init(false);
      trkdaq::DtcInterface* dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask,skip_init);

      dtc_i->fLinkMask       = link_mask;
      dtc_i->fPcieAddr       = pcie_addr;
      dtc_i->fEnabled        = enabled;
        
      dtc_i->fSampleEdgeMode = _odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
      dtc_i->fEmulateCfo     = _odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);

      dtc_i->fRocReadoutMode = roc_readout_mode;
      dtc_i->fEventMode      = event_mode;
        
      TLOG(TLVL_DEBUG) << "roc_readout_mode:"  << dtc_i->fRocReadoutMode
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

  std::initializer_list<const char*> dtc_names = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};

  const std::string node_path     = "/Equipment/"+TMFeEquipment::fEqName;
  midas::odb odb_node(node_path);
  
  const std::string settings_path = node_path+"/Settings";
  midas::odb odb_settings(node_path+"/Settings");
//-----------------------------------------------------------------------------
// DTCs and ROCs
//-----------------------------------------------------------------------------
  for (int idtc=0; idtc<2; idtc++) {
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
  }

  midas::odb::set_debug(false);
}

//-----------------------------------------------------------------------------
void TEquipmentNode::ReadDtcMetrics() {
  char   text[200];
  
  //  double t  = TMFE::GetTime();
  // midas::odb::set_debug(true);

  std::string node_path = "/Equipment/"+TMFeEquipment::fEqName;
  
  for (int idtc=0; idtc<2; idtc++) {
    trkdaq::DtcInterface* dtc_i = fDtc_i[idtc];
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
        for (int ilink=0; ilink<6; ilink++) {
          if (dtc_i->LinkEnabled(ilink)) {

            if (_monitorRocRegisters) {

              std::vector<uint32_t>  roc_reg;
              roc_reg.reserve(RocRegisters.size());

              try {
                for (const int reg : RocRegisters) {
                  // ROC registers store 16-bit words, don't know how to declare an array
                  // of shorts for ODBXX, use uint32_t
                  uint32_t dat = dtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
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
              
              try { 
                std::vector<uint16_t> spi_raw_data;
                struct trkdaq::TrkSpiData_t   spi;
                dtc_i->ReadSpiData   (ilink,spi_raw_data,0);
                dtc_i->ConvertSpiData(spi_raw_data,&spi,0);
              
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
              catch(...) {
                TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " ROC:" << ilink << " SPI";
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
                // TODO
              }
            }
          }
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