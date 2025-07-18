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
#include "midas.h"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
// read all thresholds
//-----------------------------------------------------------------------------
int TEqTrkDtc::MeasureThresholds(std::ostream& Stream) {

  int rc(0);
  
  TLOG(TLVL_DEBUG) << "-- START";

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_cmd       = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  int   print_level = odb_i->GetInteger(h_cmd,"print_level");
  int   link        = odb_i->GetInteger(h_cmd,"link"       );
  
  TLOG(TLVL_DEBUG) << "- checkpoint 0.1 Link:" << link
                   << " PcieAddr:" << _dtc_i->PcieAddr()
                   << " PrintLevel:" << print_level;

  int lnk1 = link;
  int lnk2 = lnk1+1;
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  TLOG(TLVL_DEBUG) << "-------------- lnk2, lnk2:" << lnk1 << " " << lnk2;

  std::vector<float> thr;

  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) {
      Stream << "Link:" << lnk << " is disabled" << std::endl;
      continue;
    }
    
    TLOG(TLVL_DEBUG) << " -- link:" << lnk << " enabled";
//-----------------------------------------------------------------------------
// exceptions are handled in the called function
//-----------------------------------------------------------------------------
    rc = _dtc_i->ControlRoc_ReadThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                           print_level,Stream);
    if (rc < 0) break;
//-----------------------------------------------------------------------------
// now store the output in the ODB, the packing order: (hv, cal) ... ignore 'tot')
//-----------------------------------------------------------------------------
    std::string p_panel = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                      _host_label.data(),_dtc_i->PcieAddr(),lnk);
    midas::odb o_panel(p_panel);
    for (int i=0; i<96; ++i) {
      o_panel["thr_hv_mv" ][i] = thr[3*i  ];
      o_panel["thr_cal_mv"][i] = thr[3*i+1];
    }

    _dtc_i->ControlRoc_PrintThresholds(lnk,thr,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
                                       print_level,Stream);
  }

  TLOG(TLVL_DEBUG) << "-- END, rc:" << rc;
  return rc;
}
