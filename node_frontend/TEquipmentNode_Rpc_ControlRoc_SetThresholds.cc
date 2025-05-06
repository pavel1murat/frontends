/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int TEquipmentNode::Rpc_ControlRoc_SetThresholds(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i,
                                                 std::ostream& Stream, const char* ConfName) {
  // int timeout_ms(150);
  int rc(0);
  
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_set_thresholds");

  int doit = o["Doit"] ;
  
  int lnk1 = Link;
  int lnk2 = Link+1;
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
    
    std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                          ConfName,_host_label.data(),PcieAddr,lnk);
    midas::odb o(panel_path);

    Stream << " -- link:" << lnk << std::endl;
    Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
    Stream << "---------------------------------" << std::endl;

    for (int i=0; i<96; i++) {

      int ch_mask  = o["ch_mask"][i];
      int gain_cal = o["gain_cal"][i];
      int thr_cal  = o["threshold_cal"][i];
      int gain_hv  = o["gain_hv"][i];
      int thr_hv   = o["threshold_hv"][i];
      
      Stream << std::format("{:3d} {:3d}  {:3d} {:3d} {:3d} {:3d}\n",
                            i,ch_mask,gain_cal,gain_hv,thr_cal,thr_hv);

      if (doit != 0) {
        try {
          Dtc_i->ControlRoc_SetThreshold(Link,i,0,thr_cal );
          Dtc_i->ControlRoc_SetGain     (Link,i,0,gain_cal);
        
          Dtc_i->ControlRoc_SetThreshold(Link,i,1,thr_hv );
          Dtc_i->ControlRoc_SetGain     (Link,i,1,gain_hv);
        }
        catch(...) {
          Stream << "ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
          rc = -1;
          break;
        }
      }
    }
  }

  return rc;
}
