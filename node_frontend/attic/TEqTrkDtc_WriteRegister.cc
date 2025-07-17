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
int TEqTrkDtc::WriteRegister(std::ostream& Stream) {
  
  OdbInterface* odb_i     = OdbInterface::Instance();
  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read_register");

  try {
    int      timeout_ms(150);
    uint32_t reg = odb_i->GetUInt32(h_cmd_par,"Register");
    uint32_t val = odb_i->GetUInt32(h_cmd_par,"Value"   );
    _dtc_i->fDtc->GetDevice()->write_register(reg,timeout_ms,val);

    Stream << " -- write_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
  }
  catch (...) {
    Stream << " ERROR : dtc_write_register ... BAIL OUT" << std::endl;
  }

  return 0;
}
