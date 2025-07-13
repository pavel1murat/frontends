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


int TEquipmentNode::Rpc_ControlRoc_ReadDDR(trkdaq::DtcInterface* Dtc_i, int Link, std::ostream& Stream) {
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_read_ddr");

  int block_number = o["BlockNumber"];
  // int print_level  = o["PrintLevel"];
  
  try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
    Dtc_i->ReadRocDDR(Link,block_number,Stream);
  }
  catch(...) {
    Stream << "ERROR : coudn't execute ControlRoc_ReadDDR ... BAIL OUT" << std::endl;
  }
  return 0;
}
