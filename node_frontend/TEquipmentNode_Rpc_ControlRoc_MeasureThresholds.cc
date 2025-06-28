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
#include "midas.h"
#include "odbxx.h"
#include "TRACE/tracemf.h"
#define TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
// read all thresholds
//-----------------------------------------------------------------------------
int TEquipmentNode::MeasureThresholds(ThreadContext_t& Context, std::ostream& Stream) {

  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_measure_thresholds");
  
  TLOG(TLVL_DEBUG) << "- checkpoint 0.1 Context.fLink:" << Context.fLink
                   << " Context.fPcieAddr:" << Context.fPcieAddr
                   << " Context.fPrintLevel:" << Context.fPrintLevel;

  int lnk1 = Context.fLink;
  int lnk2 = lnk1+1;
  if (Context.fLink == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << "-------------- lnk2, lnk2:" << lnk1 << " " << lnk2;

  std::vector<float> thr;

  trkdaq::DtcInterface* dtc_i = (trkdaq::DtcInterface*) fDtc_i[Context.fPcieAddr];
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (dtc_i->LinkEnabled(lnk) == 0) {
      Stream << "Link:" << lnk << " is disabled" << std::endl;
      continue;
    }
    
    TLOG(TLVL_DEBUG) << " -- link:" << lnk << " enabled";
//-----------------------------------------------------------------------------
// exceptions are handled in the called function
//-----------------------------------------------------------------------------
    rc = dtc_i->ControlRoc_ReadThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                          Context.fPrintLevel,Stream);
    if (rc < 0) break;
//-----------------------------------------------------------------------------
// now store the output in the ODB, the packing order: (hv, cal, tot)
//-----------------------------------------------------------------------------
    std::string p_panel = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                      _host_label.data(),Context.fPcieAddr,lnk);
    midas::odb o_panel(p_panel);
    for (int i=0; i<96; ++i) {
      o_panel["thr_hv_mv" ][i] = thr[3*i  ];
      o_panel["thr_cal_mv"][i] = thr[3*i+1];
    }

    dtc_i->ControlRoc_PrintThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                      Context.fPrintLevel,Stream);
  }

  TLOG(TLVL_DEBUG) << "-- END, rc:" << rc;
  return rc;
}
