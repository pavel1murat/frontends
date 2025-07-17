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

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");

  try {
    int      timeout_ms(150);
    uint32_t reg = odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val;
    _dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
    odb_i->SetUInt32(h_cmd_par,"Value",val);
    Stream << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) { Stream << " ERROR : dtc_read_register ... BAIL OUT" << std::endl; }

  return 0;
}
