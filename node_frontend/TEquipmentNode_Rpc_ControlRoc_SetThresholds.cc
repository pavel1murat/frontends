/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
//
#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
// #include "nlohmann/json.hpp"
#include "odbxx.h"
#include "TRACE/tracemf.h"

//-----------------------------------------------------------------------------
void TEquipmentNode::SetThresholds(ThreadContext_t&   Context,
                                   TEquipmentNode&    EqNode ,
                                   std::ostream&      Stream ) {

  TLOG(TLVL_DEBUG) << "-- START";
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_set_thresholds");
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.1";

  int doit        = o["Doit"] ;
  int print_level = o["PrintLevel"] ;
  
  TLOG(TLVL_DEBUG) << "-- checkpoint 0.2 Context.fLink:" << Context.fLink
                   << " Context.fPcieAddr:" << Context.fPcieAddr;

  int lnk1 = Context.fLink;
  int lnk2 = lnk1+1;
  if (Context.fLink == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  uint16_t data[4*96];

  trkdaq::DtcInterface* dtc_i = (trkdaq::DtcInterface*) EqNode.fDtc_i[Context.fPcieAddr];

  TLOG(TLVL_DEBUG) << "-- check 1";
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
// in ODB, set DTC status as busy
//-----------------------------------------------------------------------------
  std::string  dtc_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}",
                                      EqNode._host_label.data(),dtc_i->fPcieAddr);
  midas::odb o_dtc(dtc_path);
  o_dtc["Status"] = 1;

  TLOG(TLVL_DEBUG) << "-- check 1.1 dtc_path:" << dtc_path;
  for (int lnk=lnk1; lnk<lnk2; lnk++) {

    if (print_level > 0) {
      Stream << " -- link:" << lnk;
    }
    
    std::string  panel_path = std::format("{:s}/Link{:d}/DetectorElement",dtc_path.data(),lnk);
    
    TLOG(TLVL_DEBUG) << "-- check 1.2 link:" << lnk << " panel_path:" << panel_path;
    midas::odb o(panel_path);

    if (print_level > 1) {
      Stream << std::endl;
      Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
      Stream << "---------------------------------" << std::endl;
    }

    for (int i=0; i<96; i++) {
      data[i     ] = (uint16_t) o["gain_cal"     ][i];
      data[i+96*1] = (uint16_t) o["gain_hv"      ][i];
      data[i+96*2] = (uint16_t) o["threshold_cal"][i];
      data[i+96*3] = (uint16_t) o["threshold_hv" ][i];

      int ch_mask = o["ch_mask"][i];
      
      if (print_level > 1) {
        Stream << std::format("{:3d} {:3d} {:5d} {:5d} {:5d} {:5d}\n",
                              i,ch_mask,data[i],data[i+96*1],data[i+96*2],data[i+96*3]);
      }
    }
      
    if (doit != 0) {
      try {
        dtc_i->ControlRoc_SetThresholds(lnk,data);
        Stream << " : SUCCESS" ;
      }
      catch(...) {
        Stream << "ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
      }
      // TThread::Unlock();
    }
    
    Stream << std::endl;
  }
  
  o_dtc["Status"] = 0;
  TLOG(TLVL_DEBUG) << "-- END";
}
