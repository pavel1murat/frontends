///////////////////////////////////////////////////////////////////////////////
// 2026-04-11 PM: so far, there is no "pure-tracker" commands
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include "frontends/trk_cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
int TEqTracker::TestCommand(int Station, int Plane, int Panel) {
  int rc(0);
  //  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/Test");

  TLOG(TLVL_DEBUG) << std::format("-- START: Station:{} Plane:{} Panel:{}",Station,Plane,Panel);

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE         h_cmd     = odb_i->GetHandle(0,"/Mu2e/Commands/Test");
  std::string   cmd_name  = odb_i->GetString(h_cmd,"Name");
  HNDLE         h_cmd_par = odb_i->GetCmdParameterHandle(h_cmd);

  int print_level   = odb_i->GetInteger(h_cmd_par,"print_level"  );
  int return_status = odb_i->GetInteger(h_cmd_par,"return_status");
  int wait_time_ms  = odb_i->GetInteger(h_cmd_par,"wait_time_ms" );

  TLOG(TLVL_DEBUG) << std::format("command global parameters: print_level:{} return_status:{} wait_time_ms:{}",
                                  print_level,return_status,wait_time_ms);
  ss_sleep(wait_time_ms);
  
  TLOG(TLVL_DEBUG) << std::format("-- END: Station:{} Plane:{} Panel:{} rc={}",Station,Plane,Panel,rc);
  
  return rc;
}

//-----------------------------------------------------------------------------
// can be executed in a thread
// a tracker command is executed for all panels
// to process all DTC's in parallel, need each DTC to have a command buffer
// parameters of the PULSER_ON command are the same for all DTC's
// instead of passing to all DTCs, pass the parameter address 
// stored in the TRACKER command
// perhaps do not need passing the CmdParameterPath
//-----------------------------------------------------------------------------
int TEqTracker::ExecuteTrackerCommand(HNDLE H_Cmd) {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("-- START");

  std::stringstream ss;

  HNDLE h_panel(0);

  std::string cmd       = _odb_i->GetString (H_Cmd,"Name"   );
  HNDLE       h_cmd_par = _odb_i->GetCmdParameterHandle(H_Cmd);
  
  int         station   = _odb_i->GetInteger(h_cmd_par,"station");
  int         plane     = _odb_i->GetInteger(h_cmd_par,"plane"  );
  int         mnid      = _odb_i->GetInteger(h_cmd_par,"mnid"   );

  TLOG(TLVL_DEBUG) << std::format("cmd:{} station:{} plane:{} mnid:{}",cmd,station,plane,mnid);

  int first_station(-1), last_station(-1), first_plane(0), last_plane(2);

  if (mnid >= 0) {
                                        // single panel
    int hash      = (mnid/10)*10;
    std::string panel_path = std::format("PanelMap/{:03d}/MN{:03d}/Panel",hash,mnid);
    h_panel       = _odb_i->GetHandle(_handle,panel_path); 
    int slot_id   = _odb_i->GetInteger(h_panel ,"slot_id");

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
      
      first_station = _odb_i->GetInteger(_handle,"FirstStation");
      last_station  = _odb_i->GetInteger(_handle,"LastStation" );
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- START first_station:{} last_station:{} first_plane:{} last_plane:{}",
                                  first_station, last_station, first_plane, last_plane);
//-----------------------------------------------------------------------------
// it might make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int stn=first_station; stn<last_station+1; ++stn) {
    HNDLE h_station = _odb_i->GetTrackerStationHandle(stn);
    TLOG(TLVL_DEBUG+1) << std::format("station stn:{} h_station:{} enabled:{}",stn,h_station,_odb_i->GetEnabled(h_station));
    if (_odb_i->GetEnabled(h_station) == 0) continue;
    for (int pln=first_plane; pln<last_plane; ++pln) {
      HNDLE h_plane = _odb_i->GetTrackerPlaneHandle(stn,pln);
      TLOG(TLVL_DEBUG+1) << std::format("plane pln:{} h_station:{} enabled:{}",pln,h_plane,_odb_i->GetEnabled(h_plane));
      if (_odb_i->GetEnabled(h_plane) == 0) continue;
//-----------------------------------------------------------------------------
// loop over panels
//-----------------------------------------------------------------------------
      for (int pnl=0; pnl<6; ++pnl) {
        HNDLE h_panel = _odb_i->GetTrackerPanelHandle(stn,pln,pnl);
        TLOG(TLVL_DEBUG+1) << std::format("panel pln:{} pnl:{} h_station:{} enabled:{}",
                                          pln,pnl,h_plane,_odb_i->GetEnabled(h_panel));
        if (_odb_i->GetEnabled(h_panel) == 0) continue;
        std::string panel_name = _odb_i->GetString(h_panel,"Name"); // "MNXXX"
        int         panel_mnid = std::stoi(panel_name.substr(2));
        TLOG(TLVL_DEBUG+1) << std::format("panel_name:{} panel_mnid:{}",panel_name,panel_mnid);
        if ((mnid > 0) and (panel_mnid != mnid)) continue;
//-----------------------------------------------------------------------------
// execute command for a given panel, this can only be done sequentially
//-----------------------------------------------------------------------------
        if      (cmd == "print_status") {
          PanelPrintStatus(H_Cmd);
        }
        else if (cmd == "test_command") {
          TestCommand(stn,pln,pnl);
          // t.detach();
        }
      }
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}
