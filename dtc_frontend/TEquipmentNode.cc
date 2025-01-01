//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#include "dtc_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
TEquipmentNode::TEquipmentNode(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 30000;  // 30 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;

  fDtc_i[0]               = nullptr;
  fDtc_i[1]               = nullptr;  
}


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
    int pcie_addr        = _odb_i->GetPcieAddress   (hDB,h_subkey);
    int link_mask        = _odb_i->GetDtcLinkMask   (hDB,h_subkey);
    
    TLOG(TLVL_DEBUG) << "link_mask:0x" <<std::hex << link_mask << std::endl; 
    
    if (enabled) {
      // for now, disable re-initialization
      bool skip_init(true);
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
// init ODB structure no matter what
TMFeResult TEquipmentNode::InitArtdaq() {
//-----------------------------------------------------------------------------
// ARTDAQ_PARTITION_NUMBER also comes from the active run configuration
// get port number used by the TFM, don't assume the farm_manager is running locally
// the frontend has to have its own xmlrpc URL,
// TFM uses port           10000+1000*partition
// boardreaders start from 10000+1000*partition+100+1;
// init XML RPC            10000+1000*partition+11
//-----------------------------------------------------------------------------
  // int         partition   = _odb_i->GetArtdaqPartition(hDB);
  // int         port_number = 10000+1000*partition+11;
  
  char cbuf[100];
  // sprintf(cbuf,"http://%s:%i/RPC2",tfm_host.data(),port_number);
  // _xmlrpcUrl = cbuf;

  sprintf(cbuf,"%s_mon",_rpc_host.data());
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS,cbuf,"v1_0");
  xmlrpc_env_init(&_env);
//-----------------------------------------------------------------------------
// read ARTDAQ configuration from ODB
//-----------------------------------------------------------------------------
  HNDLE h_artdaq_conf = _odb_i->GetHostArtdaqConfHandle(_h_active_run_conf,_rpc_host);
  HNDLE h_component;
  KEY   component;
  for (int i=0; db_enum_key(hDB, h_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
//-----------------------------------------------------------------------------
// use the component label 
// component names:
//                   brxx - board readers
//                   ebxx - event builders
//                   dlxx - data loggers
//                   dsxx - dispatchers
//-----------------------------------------------------------------------------
    db_get_key(hDB, h_component, &component);
    printf("Subkey %d: %s, Type: %d\n", i, component.name, component.type);
    
    ArtdaqComponent_t ac;
    ac.name        = component.name;
    
    if      (ac.name.find("br") == 0) ac.type = kBoardReader;
    else if (ac.name.find("eb") == 0) ac.type = kEventBuilder;
    else if (ac.name.find("dl") == 0) ac.type = kDataLogger;
    else if (ac.name.find("ds") == 0) ac.type = kDispatcher;
    
    _odb_i->GetInteger(hDB,h_component,"XmlrpcPort",&ac.xmlprc_port);
    _odb_i->GetInteger(hDB,h_component,"Rank"      ,&ac.rank);
    // _odb_i->GetInteger(hDB,h_component,"Target"    ,&ac.target);
    _odb_i->GetInteger(hDB,h_component,"Subsystem" ,&ac.subsystem);
    _odb_i->GetInteger(hDB,h_component,"NFragmentTypes" ,&ac.n_fragment_types);

    char url[100];
    sprintf(url,"http://%s:%i/RPC2",_tfm_host.data(),ac.xmlprc_port);
    ac.xmlrpc_url  = url;
    
    _list_of_ac.push_back(ac);
  }
  
  InitArtdaqVarNames();
  
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleInit(const std::vector<std::string>& args) {

  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;

  fEqConfBuffer = "SYSTEM";
  EqSetStatus("Started...", "white");
  fMfe->Msg(MINFO, "HandleInit", std::string("Init") + " Ok!");

  cm_get_experiment_database(&hDB, NULL);

  _odb_i                      = OdbInterface::Instance(hDB);
  _h_active_run_conf          = _odb_i->GetActiveRunConfigHandle();
  std::string active_run_conf = _odb_i->GetRunConfigName(_h_active_run_conf);
//-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  _rpc_host = get_short_host_name("local");
  _tfm_host = get_full_host_name ("local");

  TLOG(TLVL_DEBUG) << "rpc_host:" << _rpc_host
                   << " active_run_conf:" << active_run_conf;

  _h_daq_host_conf = _odb_i->GetHostConfHandle(_h_active_run_conf,_rpc_host);
  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/Dtc"   ,&_monitorDtc   );
  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/Artdaq",&_monitorArtdaq);

  TLOG(TLVL_DEBUG) << "_monitorDtc:" << _monitorDtc
                   << " _monitorArtdaq:" << _monitorArtdaq;

  InitDtc();
  InitArtdaq();
  
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// 2 DTCs and their ROCs
//-----------------------------------------------------------------------------
void TEquipmentNode::InitDtcVarNames() {
  char dirname[256], var_name[128];
  
  std::initializer_list<const char*> dtc_names = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};

  const std::string node_path    ("/Equipment/mu2edaq22");
  const std::string settings_path("/Equipment/mu2edaq22/Settings");
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
    if (not midas::odb::exists(node_path+"/Settings/"+dirname)) {
      midas::odb odb_settings = {dirname,{"a"}};
      odb_settings.connect(settings_path);
      odb_settings[dirname] = dtc_var_names;
    }
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

    midas::odb   dtc_dir = {
      {"RegNames",{"a"}},
      {"RegData" ,{"a"}},
      {"ROC0",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } },
      {"ROC1",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } },
      {"ROC2",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } },
      {"ROC3",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } },
      {"ROC4",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } },
      {"ROC5",{ {"RegNames",{"a"}}, {"RegData" ,{"a"}} } }
    };
    dtc_dir.connect(node_path+"/"+dirname);
    dtc_dir["RegNames"] = dtc_var_names;

    std::vector<uint32_t> dtc_reg_data(dtc_var_names.size());
    dtc_dir["RegData"] = dtc_reg_data;
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
        midas::odb o = {dirname,{"a"}};
        o.connect(settings_path);
        o[dirname] = roc_var_names;
      }
//-----------------------------------------------------------------------------
// non-history ROC registers - counters and such - just to be looked at
//-----------------------------------------------------------------------------
      roc_var_names.clear();
      for(const int& reg : RocRegisters) {
        sprintf(var_name,"reg_%03i",reg);
        roc_var_names.push_back(var_name);
      }

      char roc_subdir[16];
      sprintf(roc_subdir,"ROC%i",ilink);
      dtc_dir[roc_subdir]["RegNames"] = roc_var_names;
      
      std::vector<uint16_t> roc_reg_data(roc_var_names.size());
      dtc_dir[roc_subdir]["RegData"] = roc_reg_data;
    }
  }
}

//-----------------------------------------------------------------------------
// init DTC reaout for a given mode at begin run
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleBeginRun(int RunNumber)  {

  HNDLE h_active_run_conf = _odb_i->GetActiveRunConfigHandle();
  int   event_mode        = _odb_i->GetEventMode     (h_active_run_conf);
  int   roc_readout_mode  = _odb_i->GetRocReadoutMode(h_active_run_conf);

  for (int i=0; i<2; i++) {
    trkdaq::DtcInterface* dtc_i = fDtc_i[i];
    if (dtc_i) {
      dtc_i->fRocReadoutMode = roc_readout_mode;
      dtc_i->fEventMode      = event_mode;

      dtc_i->InitReadout();
    }
  }
  return TMFeOk();
};

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleEndRun   (int RunNumber) {
  fMfe->Msg(MINFO, "HandleEndRun", "End run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("end_of_run %d\n", RunNumber);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandlePauseRun(int run_number) {
  fMfe->Msg(MINFO, "HandlePauseRun", "Pause run %d!", run_number);
  EqSetStatus("Stopped", "#00FF00");
    
  printf("pause_run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleResumeRun(int RunNumber) {
  fMfe->Msg(MINFO, "HandleResumeRun", "Resume run %d!", RunNumber);
  EqSetStatus("Stopped", "#00FF00");

  printf("resume_run %d\n", RunNumber);

  return TMFeOk();
}


//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleStartAbortRun(int run_number) {
  fMfe->Msg(MINFO, "HandleStartAbortRun", "Begin run %d aborted!", run_number);
  EqSetStatus("Stopped", "#00FF00");

  printf("start abort run %d\n", run_number);
    
  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleRpc(const char* cmd, const char* args, std::string& response) {
  fMfe->Msg(MINFO, "HandleRpc", "RPC cmd [%s], args [%s]", cmd, args);

  // RPC handler

  TLOG(TLVL_DEBUG) << "RPC cmd:" << cmd << " args:" << args;
  
  time_t now = time(NULL);
  char tmp[256];
  sprintf(tmp, "{ \"emoe, current_time\":[ %d, \"%s\"] }", (int)now, ctime(&now));
  
  // response = std::format("{:c} "current_time" : [{:d}, {:s}] {:c}",'{',(int) now, "emoe",'}');

  if (strcmp(cmd,"dtc_control_roc_read") == 0) {
    sprintf(tmp," -- about to learn how to do that");

    midas::odb o = {};
    o.connect("/Mu2e/Commands/Tracker/DTC/control_ROC_read");
    for (int i=0; i<2; i++) {
      trkdaq::DtcInterface* dtc_i = fDtc_i[i];
      if (dtc_i) {
        trkdaq::ControlRoc_Read_Input_t par;
        // parameters should be taken from ODB - where from? 
  
        par.adc_mode        = o["adc_mode"     ];   // -a
        par.tdc_mode        = o["tdc_mode"     ];   // -t 
        par.num_lookback    = o["num_lookback" ];   // -l 
        par.num_samples     = o["num_samples"  ];   // -s
        par.num_triggers[0] = o["num_triggers"][0]; // -T 10
        par.num_triggers[1] = o["num_triggers"][1]; //
        
        par.ch_mask[0]      = o["ch_mask"][0];
        par.ch_mask[1]      = o["ch_mask"][1];
        par.ch_mask[2]      = o["ch_mask"][2];
        par.ch_mask[3]      = o["ch_mask"][3];
        par.ch_mask[4]      = o["ch_mask"][4];
        par.ch_mask[5]      = o["ch_mask"][5];
        
        par.enable_pulser   = o["enable_pulser"];   // -p 1
        par.marker_clock    = o["marker_clock" ];   // -m 3
        par.mode            = o["mode"         ];   // 
        par.clock           = o["clock"        ];   // 

        printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
        bool update_mask(false);
        int  print_level(0);
        dtc_i->ControlRoc_Read(&par,-1,update_mask,print_level);
      }
    }
  }

  response = tmp;
  
  TLOG(TLVL_DEBUG) << "response:" << response;

  return TMFeOk();
}

//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::HandleBinaryRpc(const char* cmd, const char* args, std::vector<char>& response) {
  fMfe->Msg(MINFO, "HandleBinaryRpc", "RPC cmd [%s], args [%s]", cmd, args);

  // RPC handler
      
  response.resize(8*64);

  uint64_t* p64 = (uint64_t*)response.data();

  for (size_t i=0; i<response.size()/sizeof(p64[0]); i++) {
    *p64++ = (1<<i);
  }
      
  return TMFeOk();
}

//-----------------------------------------------------------------------------
void TEquipmentNode::ReadDtcMetrics() {
  char   text[200];
  
  //  double t  = TMFE::GetTime();
  // midas::odb::set_debug(true);
  
  for (int i=0; i<2; i++) {
    trkdaq::DtcInterface* dtc_i = fDtc_i[i];
    if (dtc_i) {
//-----------------------------------------------------------------------------
// DTC temperature and voltages - for history 
//-----------------------------------------------------------------------------
      sprintf(text,"dtc%i",i);

      std::vector<float> dtc_tv;
      for (const int reg : DtcRegHist) {
        uint32_t val;
        float    fval;
        dtc_i->fDtc->GetDevice()->read_register(reg,100,&val); 
        if      ( reg == 0x9010) fval = (val/4096.)*503.975 - 273.15;   // temperature
        else                     fval = (val/4095.)*3.;                 // voltage
        dtc_tv.emplace_back(fval);
      }

      char buf[100];
      sprintf(buf,"dtc%i",i);
      
      midas::odb odb_dtc_tv = {{buf,{1.0f, 1.0f, 1.0f, 1.0f}}};
      odb_dtc_tv.connect("/Equipment/mu2edaq22/Variables");
            
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

      sprintf(buf,"/Equipment/mu2edaq22/DTC%i",i);
      
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

            std::vector<uint16_t>  roc_reg;
            roc_reg.reserve(RocRegisters.size());
        
            for (const int reg : RocRegisters) {
              uint16_t dat = dtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
              roc_reg.emplace_back(dat);
            }

            char buf[100];
            sprintf(buf,"/Equipment/mu2edaq22/DTC%i/ROC%i",i,ilink);

            midas::odb roc = {{"RegData",{1u}}};
            roc.connect(buf);
            roc["RegData"] = roc_reg;
          }

          if (_monitorRocSPI) {
            TLOG(TLVL_DEBUG+1) << "saving ROC:" << ilink << " SPI data";
            
            std::vector<uint16_t> spi_raw_data;
            struct trkdaq::TrkSpiData_t   spi;
            dtc_i->ReadSpiData   (i,spi_raw_data,0);
            dtc_i->ConvertSpiData(spi_raw_data,&spi,0);

            std::vector<float> roc_spi;
            
            for (int iw=0; iw<trkdaq::TrkSpiDataNWords; iw++) {
              roc_spi.emplace_back(spi.Data(iw));
            }

            char buf[100];
            sprintf(buf,"rc%i%i",i,ilink);
      
            midas::odb xx = {{buf,{1.0f}}};
            xx.connect("/Equipment/mu2edaq22/Variables");
            
            xx[buf].resize(trkdaq::TrkSpiDataNWords);
            xx[buf] = roc_spi;

            TLOG(TLVL_DEBUG+1) << "ROC:" << ilink
                             << " saved N(SPI) words:" << trkdaq::TrkSpiDataNWords;
          }
          
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
// read DTC temperatures and voltages, artdaq metrics
// read ARTDAQ metrics only when running
//-----------------------------------------------------------------------------
void TEquipmentNode::HandlePeriodic() {

  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/Dtc"         ,&_monitorDtc         );
  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/Artdaq"      ,&_monitorArtdaq      );
  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/RocSPI"      ,&_monitorRocSPI      );
  _odb_i->GetInteger(hDB,_h_daq_host_conf,"Monitor/RocRegisters",&_monitorRocRegisters);

  TLOG(TLVL_DEBUG+1) << "_monitorDtc:" << _monitorDtc
                     << " _monitorRocSPI:" << _monitorRocSPI
                     << " _monitorRocRegisters:" << _monitorRocRegisters
                     << " _monitorArtdaq:" << _monitorArtdaq
                     << " TMFE::Instance()->fStateRunning:" << TMFE::Instance()->fStateRunning; 

  if (_monitorDtc) {
    ReadDtcMetrics   ();
  }

  if (_monitorArtdaq and TMFE::Instance()->fStateRunning) {
    ReadArtdaqMetrics();
  }
  
  char status[256];
  sprintf(status, "OK");
  EqSetStatus(status, "#00FF00");
}
