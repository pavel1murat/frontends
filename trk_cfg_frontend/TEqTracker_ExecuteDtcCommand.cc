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
int TEqTracker::ExecuteDtcCommand(HNDLE hTrkCmd) { // const std::string& Cmd) {
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
//-----------------------------------------------------------------------------
// pass address of parameters stored in the tracker command tree
//-----------------------------------------------------------------------------
      HNDLE h_dtc_cmd     = odb_i->GetDtcCmdHandle   (node,pcie_addr);

      int lnk = -1;
      if (mnid >= 0) {
        lnk = odb_i->GetInteger(h_panel,"Link");
        TLOG(TLVL_DEBUG+1) << std::format("   link lnk:{} h_panel:{} enabled:{}",pln,h_panel,odb_i->GetEnabled(h_panel));
        if (odb_i->GetEnabled(h_panel) == 0) continue;
      }

      TLOG(TLVL_DEBUG+1) << std::format("node:{} pcie_addr:{} link:{}",node,pcie_addr,lnk);

      int finished = odb_i->GetInteger(h_dtc_cmd,"Finished");
      if (finished == 0) {
      }
      else if (finished == 1) {
        odb_i->SetString (h_dtc_cmd,"Name"         ,cmd);
        odb_i->SetString (h_dtc_cmd,"ParameterPath",cmd_parameter_path);
        
        odb_i->SetInteger(h_dtc_cmd,"link"         ,lnk);
        odb_i->SetInteger(h_dtc_cmd,"ReturnCode"   , 0);
      // 'Finished' is set by the frontend
      //      odb_i->SetInteger(h_dtc_cmd,"Finished"     , 0);
//-----------------------------------------------------------------------------
// and trigger the execution of a 'per-DTC' command
//-----------------------------------------------------------------------------
        odb_i->SetInteger(h_dtc_cmd,"Run"          , 1);
      }
      else {
//-----------------------------------------------------------------------------
// dont know what to do - unknown value of 'Finished' - generate diagnostics and
// move on to processing the next panel
//-----------------------------------------------------------------------------
        TLOG(TLVL_ERROR) << std::format("Finished:{}",finished);
        rc += -1;
      }
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}
