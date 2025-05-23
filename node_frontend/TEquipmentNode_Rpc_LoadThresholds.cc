/////////////////////////////////////////////////////////////////////////////
// PM
/////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
//
#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"
#include "TRACE/tracemf.h"

//-----------------------------------------------------------------------------
void TEquipmentNode::LoadThresholds(ThreadContext_t&   Context,
                                    std::ostream&      Stream ) {
  TLOG(TLVL_DEBUG) << "-- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/control_ROC_load_thresholds");
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  int doit        = o_cmd["Doit"      ] ;
  int print_level = o_cmd["PrintLevel"] ;
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 Context.fLink:" << Context.fLink
                   << " Context.fPcieAddr:" << Context.fPcieAddr;

  int lnk1 = Context.fLink;
  int lnk2 = lnk1+1;
  if (Context.fLink == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  trkdaq::DtcInterface* dtc_i = (trkdaq::DtcInterface*) fDtc_i[Context.fPcieAddr];

  TLOG(TLVL_DEBUG) << "-- check 1";
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
// in ODB, set DTC status as busy
//-----------------------------------------------------------------------------
  std::string config_dir = _odb_i->GetConfigDir();
    
  midas::odb o_tracker("/Mu2e/ActiveRunConfiguration/Tracker");
  std::string thresholds_dir = o_tracker["ThresholdsDir"];

  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      _host_label.data(),dtc_i->fPcieAddr);
  midas::odb o_dtc(dtc_path);
  o_dtc["Status"] = 1;

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path
                   << " thresholds_dir:" << thresholds_dir;
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {

    Stream << " -- link:" << lnk;
    
    try {
      std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    
      TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path;

      midas::odb o_panel(panel_path);
      std::string panel_name = o_panel["Name"];

      TLOG(TLVL_DEBUG) << "-- check 1.3 panel_name:" << panel_name;

      std::string panel_map_path = std::format("/Mu2e/ActiveRunConfiguration/Tracker/PanelMap/{:s}",panel_name.data());

      TLOG(TLVL_DEBUG) << "-- check 1.35 panel_map_path:" << panel_map_path;
      midas::odb o_panel_map(panel_map_path);
      int station = o_panel_map["Station"];

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
    
   
      for (auto& elm : jf.items()) {
        nlohmann::json val = elm.value();
        int ich            = val["channel"  ];
        int gain           = val["gain"     ];
        int thr            = val["threshold"];
        std::string type   = val["type"     ];
        
        TLOG(TLVL_DEBUG) << "-- check 1.6 jf.ich:" << ich << " type:" << type
                         << " gain:" << gain << " thr:" << thr;
        
        if (print_level > 1) {
          Stream << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;
        }
        
        if (doit != 0) {
          if      (type == "cal") {
            o_panel["threshold_cal"][ich] = thr;
            o_panel["gain_cal"     ][ich] = gain;
          }
          else if (type == "hv" ) {
            o_panel["threshold_hv" ][ich] = thr;
            o_panel["gain_hv"      ][ich] = gain;
          }
        }
      }
      Stream << " SUCCESS" << std::endl;
    }
    catch (...) {
      Stream << " ERROR" << std::endl;
    }
    
  }
  
  o_dtc["Status"] = 0;
  TLOG(TLVL_DEBUG) << "-- END";
}
