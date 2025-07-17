/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
//
#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
// #include "nlohmann/json.hpp"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
int TEqTrkDtc::SetThresholds(std::ostream& Stream ) {

  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();
  
  //  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_set_thresholds");

  HNDLE h_cmd = odb_i->GetHandle(0,"/Mu2e/Commands/Tracker/DTC/control_roc_set_thresholds");
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  // int doit        = o["doit"] ;
  // int print_level = o["print_level"] ;

  int doit        = odb_i->GetInteger(h_cmd,"doit"       );
  int print_level = odb_i->GetInteger(h_cmd,"print_level");
  int link        = odb_i->GetInteger(h_cmd,"link"       );
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 link:" << link
                   << " PcieAddr:" << _dtc_i->PcieAddr();

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
  // midas::odb o_dtc(dtc_path);
  // o_dtc["Status"] = 1;

  HNDLE h_dtc = odb_i->GetDtcConfigHandle(_host_label,_dtc_i->PcieAddr());
  odb_i->SetStatus(h_dtc,1);

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path;
  for (int lnk=lnk1; lnk<lnk2; lnk++) {

    if (print_level > 0) {
      Stream << " -- link:" << lnk;
    }
    
    std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    HNDLE        h_panel    = odb_i->GetHandle(0,panel_path);
    
    TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path << " h_panel:" << h_panel;

    // midas::odb o(panel_path);

    if (print_level > 1) {
      Stream << std::endl;
      Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
      Stream << "---------------------------------" << std::endl;
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
        Stream << std::format("{:3d} {:3d} {:5d} {:5d} {:5d} {:5d}\n",
                              i,ch_mask[i],data[i],data[i+96*1],data[i+96*2],data[i+96*3]);
      }
    }
      
    if (doit != 0) {
      try {
        _dtc_i->ControlRoc_SetThresholds(lnk,data);
        Stream << " : SUCCESS" ;
      }
      catch(...) {
        Stream << "ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
      }
    }
    
    Stream << std::endl;
  }
  
  odb_i->SetStatus(h_dtc,0);
  
  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}
