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
int TEqTrkDtc::DigiRW(std::ostream& Stream) {

  TLOG(TLVL_DEBUG) << "--- START";

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_cmd = odb_i->GetDtcCommandHandle(HostLabel(),_dtc_i->PcieAddr());

  std::string cmd            = odb_i->GetString(h_cmd,"Name");
  std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
  
  HNDLE h_cmd_par            = odb_i->GetHandle(0,parameter_path);
    
  trkdaq::ControlRoc_DigiRW_Input_t  par;
  trkdaq::ControlRoc_DigiRW_Output_t pout;
    
  par.rw           = odb_i->GetInteger(h_cmd_par,"rw");         //
  par.hvcal        = odb_i->GetInteger(h_cmd_par,"hvcal");      //
  par.address      = odb_i->GetInteger(h_cmd_par,"address");    //
  par.data[0]      = odb_i->GetInteger(h_cmd_par,"data[0]"]; //
  par.data[1]      = odb_i->GetInteger(h_cmd_par,"data[1]"]; //

  int  print_level = odb_i->GetInteger(h_cmd_par,"print_level"];
  int  link        = odb_i->GetInteger(h_cmd_par,"link"];
   
  printf("dtc_i->fLinkMask: 0x%04x\n",_dtc_i->fLinkMask);
  _dtc_i->ControlRoc_DigiRW(&par,&pout,link,print_level,ss);
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}
