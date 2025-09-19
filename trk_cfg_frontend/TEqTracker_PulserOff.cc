///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include "frontends/trk_cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
int TEqTracker::PulserOff(const std::string& CmdParameterPath) {
  int rc(0);
  std::string  cmd("pulser_off");
                  
  TLOG(TLVL_DEBUG) << "--- START TEqTracker::" << __func__; 

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_trk_config = odb_i->GetTrackerConfigHandle();
  int first_station  = odb_i->GetInteger(h_trk_config,"FirstStation");
  int last_station   = odb_i->GetInteger(h_trk_config,"LastStation" );

  TLOG(TLVL_DEBUG) << " CmdParameterPath:" << CmdParameterPath;

  HNDLE h_trk_cmd     = odb_i->GetTrackerCmdHandle();
  HNDLE h_trk_cmd_par = odb_i->GetTrackerCmdParameterHandle(cmd);

  // the same parameters for all DTCs
  std::string cmd_parameter_path = odb_i->GetString(h_trk_cmd,"ParameterPath");
  
  int   link          = -1;  // tracker commands always operate on all enabled links
  int   print_level   = odb_i->GetInteger(h_trk_cmd_par,"print_level");
//-----------------------------------------------------------------------------
// loop over all active ROCs and execute 'PULSER_ON'
// it woudl make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int is=first_station; is<last_station+1; ++is) {
    HNDLE h_station = odb_i->GetTrackerStationHandle(is);
    if (odb_i->GetEnabled(h_station) == 0) continue;
    
    for (int pln=0; pln<2; ++pln) {

      HNDLE h_plane    = odb_i->GetTrackerPlaneHandle(is,pln);
      HNDLE h_dtc      = odb_i->GetHandle(h_plane,"DTC");
      int pcie_addr    = odb_i->GetPcieAddress(h_dtc);   // o_dtc  ["PcieAddress"];
      std::string node = odb_i->GetDtcHostLabel(h_dtc);

      TLOG(TLVL_DEBUG) << "cmd_parameter_path:" << cmd_parameter_path
                       << " is:" << is << " pln:" << pln
                       << " pcie_addr:" << pcie_addr << " link:" << link
                       << " print_level:" << print_level;

      HNDLE h_dtc_cmd = odb_i->GetDtcCmdHandle(node,pcie_addr);

      odb_i->SetString (h_dtc_cmd,"Name"         ,cmd);
      odb_i->SetString (h_dtc_cmd,"ParameterPath",cmd_parameter_path);
      odb_i->SetInteger(h_dtc_cmd,"link"         ,-1);
      
      HNDLE h_dtc_cmd_par = odb_i->GetDtcCmdParameterHandle(node,pcie_addr,cmd);
      odb_i->SetInteger(h_dtc_cmd_par,"print_level",0);
//-----------------------------------------------------------------------------
// finally, execute the command. THe loop is executed fast, so need to wait
// for the DTC's to report that they are finished
//-----------------------------------------------------------------------------
      odb_i->SetStatus(h_dtc,1); // BUSY
      
      odb_i->SetInteger(h_dtc_cmd,"Finished",0);
      odb_i->SetInteger(h_dtc_cmd,"Run"     ,1);
    }
  }
  TLOG(TLVL_DEBUG) << "--- END TEqTracker::" << __func__ << " rc:" << rc;
  return rc;
}
