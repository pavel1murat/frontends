///////////////////////////////////////////////////////////////////////////////

#include "node_frontend/TEqCrvDtc.hh"
#include "utils/OdbInterface.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqCrvDtc"
//-----------------------------------------------------------------------------
// 'skip_dtc_init' is common for all DTCs, may become obsolete
//-----------------------------------------------------------------------------
TEqCrvDtc::TEqCrvDtc(HNDLE H_RunConf, HNDLE H_Dtc) : TMu2eEqBase () {

  OdbInterface* odb_i = OdbInterface::Instance();

  int pcie_addr        = odb_i->GetDtcPcieAddress(H_Dtc);
  int link_mask        = odb_i->GetLinkMask      (H_Dtc);
  int skip_dtc_init    = odb_i->GetSkipDtcInit   (H_RunConf);
  
  TLOG(TLVL_DEBUG) << "link_mask:0x" << std::hex << link_mask << std::dec << " pcie_addr:" << pcie_addr;
  
  _dtc_i = mu2edaq::DtcInterface::Instance(pcie_addr,link_mask,skip_dtc_init);
  _dtc_i->fIsCrv     = 1;
  _dtc_i->fSubsystem = mu2edaq::kCRV;

  _logfile = "/home/mu2etrk/test_stand/experiments/test_025/crvdtc.log"; // TODO: to come from config
  
}

//-----------------------------------------------------------------------------
TEqCrvDtc::~TEqCrvDtc() {
}

//-----------------------------------------------------------------------------
TMFeResult TEqCrvDtc::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
int TEqCrvDtc::InitVarNames() {
  return 0;
}


//-----------------------------------------------------------------------------
int TEqCrvDtc::ReadMetrics() {
  return 0;
}
