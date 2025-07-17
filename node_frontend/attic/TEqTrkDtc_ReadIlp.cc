/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEqTrkDtc"

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadIlp(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_ilp");

  int link         = o["link"       ];
  int print_level  = o["print_level"];
  
  try         {
    std::vector<uint16_t>   data;
    _dtc_i->ControlRoc_ReadIlp(data,link,print_level,Stream);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "failed ControlRoc_ReadIlp for link:" << link;
    Stream << "ERROR : coudn't read ILP ... BAIL OUT" << std::endl;
    rc = -1;
  }
  return rc;
}
