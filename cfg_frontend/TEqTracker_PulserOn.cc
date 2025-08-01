///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include "frontends/cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
// a tracker command is executed for all panels
// to process all DTC's in parallel, need each DTC to have a command buffer
// parameters of the PULSER_ON command are the same for all DTC's
// instead of passing to all DTCs, pass the parameter address 
// stored in the TRACKER command
// perhaps do not need passing the CmdParameterPath
//-----------------------------------------------------------------------------
int TEqTracker::PulserOn(const std::string& CmdParameterPath) {
  int rc(0);
  std::string cmd("pulser_on");
  
  TLOG(TLVL_DEBUG) << "--- START TEqTracker::" << __func__; 

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_trk_cfg   = odb_i->GetTrackerConfigHandle();
  int first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
  int last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" );

  odb_i->SetInteger(h_trk_cfg,"Status",1);

  TLOG(TLVL_DEBUG) << " CmdParameterPath:" << CmdParameterPath;

  //  HNDLE h_trk_cmd     = odb_i->GetTrackerCmdHandle();

  std::string trk_cmd_parameter_path = odb_i->GetTrackerCmdParameterPath(cmd);
//-----------------------------------------------------------------------------
// loop over all active DTCs and execute 'PULSER_ON'
// it might make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int is=first_station; is<last_station+1; ++is) {
    HNDLE h_station = odb_i->GetTrackerStationHandle(is);
    if (odb_i->GetEnabled(h_station) == 0) continue;
    for (int pln=0; pln<2; ++pln) {
      HNDLE h_plane = odb_i->GetTrackerPlaneHandle(is,pln);
      if (odb_i->GetEnabled(h_plane) == 0) continue;
//-----------------------------------------------------------------------------
// at this point, instead of looping over the panels, need to find the DTC
// and pass parameters to it with link=-1
//-----------------------------------------------------------------------------
      HNDLE       h_dtc     = odb_i->GetHandle(h_plane,"DTC");
      int         pcie_addr = odb_i->GetDtcPcieAddress(h_dtc);
      std::string node      = odb_i->GetDtcHostLabel  (h_dtc);

      TLOG(TLVL_DEBUG) << "node:" << node << " pcie_addr:" << pcie_addr;
//-----------------------------------------------------------------------------
// pass address of parameters stored in the tracker command tree
//-----------------------------------------------------------------------------
      HNDLE h_dtc_cmd     = odb_i->GetDtcCmdHandle   (node,pcie_addr);

      odb_i->SetString (h_dtc_cmd,"Name"         ,"pulser_on");
      odb_i->SetString (h_dtc_cmd,"ParameterPath",trk_cmd_parameter_path);
      odb_i->SetInteger(h_dtc_cmd,"link"         ,-1);
      odb_i->SetInteger(h_dtc_cmd,"ReturnCode"   , 0);
      odb_i->SetInteger(h_dtc_cmd,"Finished"     , 0);
//-----------------------------------------------------------------------------
// and trigger the execution
//-----------------------------------------------------------------------------
      odb_i->SetInteger(h_dtc_cmd,"Run"          , 1);
    }
  }
  
  TLOG(TLVL_DEBUG) << "--- END TEqTracker::" << __func__ << " rc:" << rc;
  return rc;
}
