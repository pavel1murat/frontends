/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "odbxx.h"

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
// this function needs to define which type of equipment is being talked to and forward
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::Rpc_CRV(mu2edaq::DtcInterface* Dtc_i, HNDLE hDtc, const char* cmd, const char* args, std::ostream& Stream) {
  fMfe->Msg(MINFO, "Rpc_CRV", "RPC cmd [%s], args [%s]", cmd, args);

  ss.str("");

  return TMFeOk();
}
