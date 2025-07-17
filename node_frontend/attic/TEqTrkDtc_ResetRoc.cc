/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEqTrkDtc"
//-----------------------------------------------------------------------------
int TEqTrkDtc::ResetRoc(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/reset_roc");

  int link         = o["link"       ];
  int print_level  = o["print_level"];
  
  rc = _dtc_i->ResetLink(link);

  if (rc == 0) Stream << " -- reset_roc OK";
  else         Stream << " -- ERROR: failed reset_roc link:" << link << std::endl;
  
  return rc;
}
