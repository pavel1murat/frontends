/////////////////////////////////////////////////////////////////////////////
// PM
// this has to be done differently - via a loop over the stations/planes/panes
// of active configuration
// and the command itself has to be a tracker initialization command
/////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
//
#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"
#include "TRACE/tracemf.h"

#define TRACE_NAME "TEqTrkDtc"
//-----------------------------------------------------------------------------
int TEqTrkDtc::LoadThresholds(std::ostream& Stream) {
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE h_cmd = odb_i->GetHandle(0,"/Mu2e/Commands/Tracker/DTC/control_roc_load_thresholds");
  int doit        = odb_i->GetInteger(h_cmd,"doit"       );
  int print_level = odb_i->GetInteger(h_cmd,"print_level");
  int link        = odb_i->GetInteger(h_cmd,"link"       );
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 Link:" << link
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
// get config directory on disk
//-----------------------------------------------------------------------------
  std::string config_dir = odb_i->GetConfigDir();
    
  HNDLE h_tracker = odb_i->GetHandle(0,"/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration");
  std::string thresholds_dir = odb_i->GetString(h_tracker,"ThresholdsDir");

  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      _host_label.data(),_dtc_i->fPcieAddr);

  HNDLE h_dtc = odb_i->GetHandle(0,dtc_path);
  odb_i->SetStatus(h_dtc,1);

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path
                   << " thresholds_dir:" << thresholds_dir;
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {

    Stream << " -- link:" << lnk;
    
    try {
      std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    
      TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path;

      HNDLE h_panel          = odb_i->GetHandle(0,panel_path);
      std::string panel_name = odb_i->GetString(h_panel,"Name");
      
      int mnid = atoi(panel_name.substr(2).data());
      int sdir = (mnid/10)*10;

      TLOG(TLVL_DEBUG) << "-- check 1.3 panel_name:" << panel_name << " sdir:" << sdir;

      int station = odb_i->GetInteger(h_panel,"Station");
      TLOG(TLVL_DEBUG) << "-- check 1.4 station:" << station;
      
      std::string fn = std::format("{:}/tracker/station_{:02d}/{:s}/{:s}.json",
                                   config_dir.data(),station,thresholds_dir.data(),panel_name.data());
    
      TLOG(TLVL_DEBUG) << "-- check 1.5 fn:" << fn;

      std::ifstream ifs(fn);
      nlohmann::json jf = nlohmann::json::parse(ifs);
    
    
      if (print_level > 1) {
        Stream << std::endl;
        Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
        Stream << "---------------------------------" << std::endl;
      }
    
      uint16_t thr_cal[96], thr_hv[96], gain_cal[96], gain_hv[96];

      odb_i->GetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
      odb_i->GetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);
      odb_i->GetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
      odb_i->GetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
      
      for (auto& elm : jf.items()) {
        nlohmann::json val = elm.value();
        uint16_t ich       = val["channel"  ];
        uint16_t gain      = val["gain"     ];
        uint16_t thr       = val["threshold"];
        std::string type   = val["type"     ];
        
        TLOG(TLVL_DEBUG) << "-- check 1.6 jf.ich:" << ich << " type:" << type
                         << " gain:" << gain << " thr:" << thr;
        
        if (print_level > 1) {
          Stream << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;
          TLOG(TLVL_DEBUG+1) << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;
        }
        
        if (doit != 0) {
          std::string thr_key, gain_key;
          if      (type == "cal") {
            thr_cal [ich] = thr;
            gain_cal[ich] = gain;
          }
          else if (type == "hv" ) {
            thr_hv [ich] = thr;
            gain_hv[ich] = gain;
          }
        }
      }
      
      odb_i->SetArray(h_panel,"thr_cal" ,TID_WORD,thr_cal ,96);
      odb_i->SetArray(h_panel,"thr_hv"  ,TID_WORD,thr_hv  ,96);
      odb_i->SetArray(h_panel,"gain_cal",TID_WORD,gain_cal,96);
      odb_i->SetArray(h_panel,"gain_hv" ,TID_WORD,gain_hv ,96);
      
      Stream << " SUCCESS" << std::endl;
    }
    catch (...) {
      Stream << " ERROR" << std::endl;
    }
    
  }
                                      
  odb_i->SetStatus(h_dtc,0);
  
  TLOG(TLVL_DEBUG) << "-- END";

  return 0;
}
