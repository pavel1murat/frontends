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
// takes parameters frpm ODB
//-----------------------------------------------------------------------------
int TEqTrkDtc::SetCalDac(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/set_caldac");
    
  int first_channel_mask = o_cmd["first_channel_mask"];    //
  int value              = o_cmd["value"             ];    //
  int print_level        = o_cmd["print_level"       ];
  int link               = o_cmd["link"              ];

  int lnk1(link), lnk2(link+1);
  if (link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (_dtc_i->LinkEnabled(lnk) == 0) continue ;
    _dtc_i->ControlRoc_SetCalDac(lnk,first_channel_mask,value,print_level,Stream);
  }
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}
