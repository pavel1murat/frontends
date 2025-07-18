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
int TEqTrkDtc::ReadDDR(std::ostream& Stream) {
  int rc(0);
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_ddr");

  int block_number = o["block_number"];
  int link         = o["link"        ];
  // int print_level  = o["print_level"];
  
  try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
    _dtc_i->ReadRocDDR(link,block_number,Stream);
  }
  catch(...) {
    TLOG(TLVL_ERROR) << "failed ControlRoc_ReadDDR for link:" << link;
    Stream << "ERROR : coudn't execute ControlRoc_ReadDDR ... BAIL OUT" << " link:" << link << std::endl;
    rc = -1;
  }
  return rc;
}
