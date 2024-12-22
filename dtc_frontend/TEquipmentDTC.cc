//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
#include "dtc_frontend/TEquipmentDTC.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentDTC"

//-----------------------------------------------------------------------------
TEquipmentDTC::TEquipmentDTC(const char* eqname, const char* eqfilename): TMFeEquipment(eqname,eqfilename) {
  fEqConfEventID          = 3;
  fEqConfPeriodMilliSec   = 10000;  // 10 sec ?
  fEqConfLogHistory       = 1;
  fEqConfWriteEventsToOdb = true;

  fDtc_i[0]  = nullptr;
  fDtc_i[1]  = nullptr;
  
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i         = OdbInterface::Instance(hDB);
  std::string active_run_conf = odb_i->GetActiveRunConfig(hDB);
  HNDLE h_active_run_conf     = odb_i->GetRunConfigHandle(hDB,active_run_conf);
  //-----------------------------------------------------------------------------
// now go to /Mu2e/RunConfigurations/$detector_conf/DAQ to get a list of 
// nodes/DTC's to be monitored 
// MIDAS 'host_name' could be 'local'..
//-----------------------------------------------------------------------------
  std::string rpc_host  = get_short_host_name("local");

  TLOG(TLVL_DEBUG+2) << "rpc_host:" << rpc_host << " active_run_conf:" << active_run_conf;

  HNDLE h_daq_host_conf = odb_i->GetDaqHostHandle(hDB,h_active_run_conf,rpc_host);
//-----------------------------------------------------------------------------
// DTC is the equipment, two are listed in the header, both should be listed in ODB
//-----------------------------------------------------------------------------
  HNDLE h_subkey;
  KEY   subkey;

  // int      cfo_pcie_addr(-1);
  // uint64_t nevents      (0);
  // int      ew_length    (0);
  // uint64_t first_ew_tag (0);

  for (int i=0; db_enum_key(hDB,h_daq_host_conf,i,&h_subkey) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq' folder
//-----------------------------------------------------------------------------
    db_get_key(hDB,h_subkey,&subkey);
    
    TLOG(TLVL_DEBUG) << "subkey.name:" << subkey.name;
    
    if (strstr(subkey.name,"DTC") != subkey.name)           continue;
    
    int enabled          = odb_i->GetDtcEnabled    (hDB,h_subkey);
    int pcie_addr        = odb_i->GetPcieAddress   (hDB,h_subkey);
    int link_mask        = odb_i->GetDtcLinkMask   (hDB,h_subkey);
    
    TLOG(TLVL_DEBUG) << "link_mask:0x" <<std::hex << link_mask << std::endl; 
    
    if (enabled) {
      trkdaq::DtcInterface* dtc_i = trkdaq::DtcInterface::Instance(pcie_addr,link_mask);
      
      dtc_i->fRocReadoutMode = odb_i->GetDtcReadoutMode   (hDB,h_subkey);
      dtc_i->fSampleEdgeMode = odb_i->GetDtcSampleEdgeMode(hDB,h_subkey);
      dtc_i->fEmulateCfo     = odb_i->GetDtcEmulatesCfo   (hDB,h_subkey);
      
      TLOG(TLVL_DEBUG) << "readout_mode:"      << dtc_i->fRocReadoutMode
                       << " sample_edge_mode:" << dtc_i->fSampleEdgeMode
                       << " emulate_cfo:"      << dtc_i->fEmulateCfo;
      
      fDtc_i[pcie_addr]   = dtc_i;
    }
  }
}

//-----------------------------------------------------------------------------
// overloaded function of TMFeEquipment : 2 DTCs
//-----------------------------------------------------------------------------
TMFeResult TEquipmentDTC::HandleInit(const std::vector<std::string>& args) {
  fEqConfReadOnlyWhenRunning = false;
  fEqConfWriteEventsToOdb    = true;
  //fEqConfLogHistory = 1;
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// read DTC temperatures and voltages
//-----------------------------------------------------------------------------
void TEquipmentDTC::HandlePeriodic() {
    
  char   buf[1024], text[200];
  
  //  double t  = TMFE::GetTime();

  for (int i=0; i<2; i++) {
    if (fDtc_i[i]) {
      ComposeEvent(buf, sizeof(buf));
      BkInit      (buf, sizeof(buf));

      sprintf(text,"dtc%i",i);
      double* ptr = (double*) BkOpen(buf, text, TID_DOUBLE);

      for (int ireg=0; ireg<kNRegHist; ireg++) {
        uint32_t val;
        fDtc_i[i]->fDtc->GetDevice()->read_register(RegHist[ireg],100,&val); 
        if      ( RegHist[i] == 0x9010) *ptr = (val/4096.)*503.975 - 273.15;   // temperature
        else                            *ptr = (val/4095.)*3.;                 // voltage
        ptr++;
      }

      BkClose    (buf,ptr);
      EqSendEvent(buf);
    }
  }
  
  char status[256];
  sprintf(status, "OK");
  EqSetStatus(status, "#00FF00");
}
