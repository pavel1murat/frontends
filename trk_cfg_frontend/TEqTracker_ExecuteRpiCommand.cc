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
// a tracker command is executed for all panels
// to process all DTC's in parallel, need each DTC to have a command buffer
// parameters of the PULSER_ON command are the same for all DTC's
// instead of passing to all DTCs, pass the parameter address 
// stored in the TRACKER command
// perhaps do not need passing the CmdParameterPath
//-----------------------------------------------------------------------------
int TEqTracker::ExecuteRpiCommand(HNDLE hTrkCmd) { // const std::string& Cmd) {
  int rc(0);

  OdbInterface* odb_i = OdbInterface::Instance();
  HNDLE h_trk_cfg   = odb_i->GetTrackerConfigHandle();
  HNDLE h_panel(0);

  std::string cmd = odb_i->GetString (hTrkCmd,"Name"   );
  int station     = odb_i->GetInteger(hTrkCmd,"station");
  int plane       = odb_i->GetInteger(hTrkCmd,"plane"  );
  int mnid        = odb_i->GetInteger(hTrkCmd,"mnid"   );
  
  TLOG(TLVL_DEBUG) << std::format("-- START cmd:{} station:{} plane:{} mnid:{}",cmd,station,plane,mnid);

  int first_station(-1), last_station(-1), first_plane(0), last_plane(2);

  if (mnid >= 0) {
                                        // single panel
    int hash = (mnid/10)*10;
    std::string panel_path = std::format("PanelMap/{:03d}/MN{:03d}/Panel",hash,mnid);
    h_panel       = odb_i->GetHandle(h_trk_cfg,panel_path); 
    int slot_id   = odb_i->GetInteger(h_panel ,"slot_id");

    station     = (slot_id/10)/2;
    plane       = (slot_id/10)%2;
    first_station = station;
    last_station  = station;

    first_plane   = plane;
    last_plane    = first_plane+1;
  }
  else {
//-----------------------------------------------------------------------------
// could be a plane, a station, or the whole tracker
//-----------------------------------------------------------------------------
    if (plane >= 0) {
                                        // a single plane, nothing needs to be redefined
    }
    else if (station >= 0) {
                                        // a single station, both planes
      first_station = station;
      last_station  = station;
    }
    else {
                                        // all stations, all active panels
      
      first_station = odb_i->GetInteger(h_trk_cfg,"FirstStation");
      last_station  = odb_i->GetInteger(h_trk_cfg,"LastStation" );
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- START first_station:{} last_station:{} first_plane:{} last_plane:{}",
                                  first_station, last_station, first_plane, last_plane);

  odb_i->SetInteger(h_trk_cfg,"Status",1);

  std::string cmd_parameter_path = odb_i->GetTrackerCmdParameterPath(cmd);
//-----------------------------------------------------------------------------
// loop over all active DTCs and execute 'PULSER_ON'
// it might make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int is=first_station; is<last_station+1; ++is) {
    HNDLE h_station = odb_i->GetTrackerStationHandle(is);
    TLOG(TLVL_DEBUG+1) << std::format("  station is:{} h_station:{} enabled:{}",is,h_station,odb_i->GetEnabled(h_station));
    if (odb_i->GetEnabled(h_station) == 0) continue;
    for (int pln=first_plane; pln<last_plane; ++pln) {
      HNDLE h_plane = odb_i->GetTrackerPlaneHandle(is,pln);
      TLOG(TLVL_DEBUG+1) << std::format("   plane pln:{} h_station:{} enabled:{}",pln,h_plane,odb_i->GetEnabled(h_plane));
      if (odb_i->GetEnabled(h_plane) == 0) continue;
//-----------------------------------------------------------------------------
// at this point, instead of looping over the panels, need to find the DTC
// and pass parameters to it with link=-1
// [dangerous] assumption that we have a DTC per plane, so everything is simple
//-----------------------------------------------------------------------------
      HNDLE       h_dtc     = odb_i->GetHandle(h_plane,"DTC");
      int         pcie_addr = odb_i->GetDtcPcieAddress(h_dtc);
      std::string node      = odb_i->GetDtcHostLabel  (h_dtc);
      // tbd

      if (cmd == "reset_station_lv") ResetStationLV(cmd_parameter_path);
      else {
        TLOG(TLVL_ERROR) << std::format("unknown command:{}",cmd);
        rc = -1;
        break;
      }
      if (rc < 0) break;
    }
    if (rc < 0) break;
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}
