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

#include "TRACE/tracemf.h"
#define TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
// takes parameters frpm ODB
//-----------------------------------------------------------------------------
int TEquipmentNode::Rpc_ControlRoc_SetCalDac(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i, std::ostream& Stream, const char* ConfName) {

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/set_caldac");
    
  int first_channel_mask = o_cmd["first_channel_mask"];    //
  int value              = o_cmd["value"             ];    //
  int print_level        = o_cmd["print_level"       ];

  int lnk1(Link), lnk2(Link+1);
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (Dtc_i->LinkEnabled(lnk) == 0) continue ;
    Dtc_i->ControlRoc_SetCalDac(lnk,first_channel_mask,value,print_level,Stream);
  }
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}
