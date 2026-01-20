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
int TEqTracker::ExecuteTrackerCommand(HNDLE hTrkCmd) { // const std::string& Cmd) {
  int rc(0);
  TLOG(TLVL_DEBUG) << std::format("-- START");

  std::stringstream ss;

  OdbInterface* odb_i = OdbInterface::Instance();
  HNDLE h_trk_cfg   = odb_i->GetTrackerConfigHandle();
  HNDLE h_panel(0);

  std::string cmd                  = odb_i->GetString (hTrkCmd,"Name"   );
  std::string cmd_parameter_path   = odb_i->GetString (hTrkCmd,"ParameterPath");
  
  HNDLE       h_cmd_parameter_path = odb_i->GetHandle(0,cmd_parameter_path);
  
  int station                      = odb_i->GetInteger(h_cmd_parameter_path,"station");
  int plane                        = odb_i->GetInteger(h_cmd_parameter_path,"plane"  );
  int mnid                         = odb_i->GetInteger(h_cmd_parameter_path,"mnid"   );

  // HNDLE cmd_parameter_path = odb_i->GetHandle(hTrkCmd,"ParameterPath");

  TLOG(TLVL_DEBUG) << std::format("cmd:{} station:{} plane:{} mnid:{}",cmd,station,plane,mnid);

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
//-----------------------------------------------------------------------------
// loop over all active DTCs and execute 'PULSER_ON'
// it might make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int stn=first_station; stn<last_station+1; ++stn) {
    HNDLE h_station = odb_i->GetTrackerStationHandle(stn);
    TLOG(TLVL_DEBUG+1) << std::format("station stn:{} h_station:{} enabled:{}",stn,h_station,odb_i->GetEnabled(h_station));
    if (odb_i->GetEnabled(h_station) == 0) continue;
    for (int pln=first_plane; pln<last_plane; ++pln) {
      HNDLE h_plane = odb_i->GetTrackerPlaneHandle(stn,pln);
      TLOG(TLVL_DEBUG+1) << std::format("plane pln:{} h_station:{} enabled:{}",pln,h_plane,odb_i->GetEnabled(h_plane));
      if (odb_i->GetEnabled(h_plane) == 0) continue;
//-----------------------------------------------------------------------------
// loop over panels
//-----------------------------------------------------------------------------
      for (int pnl=0; pnl<6; ++pnl) {
        HNDLE h_panel = odb_i->GetTrackerPanelHandle(stn,pln,pnl);
        TLOG(TLVL_DEBUG+1) << std::format("panel pln:{} pnl:{} h_station:{} enabled:{}",
                                          pln,pnl,h_plane,odb_i->GetEnabled(h_panel));
        if (odb_i->GetEnabled(h_panel) == 0) continue;
        std::string panel_name = odb_i->GetString(h_panel,"Name"); // "MNXXX"
        int         panel_mnid = std::stoi(panel_name.substr(2));
        TLOG(TLVL_DEBUG+1) << std::format("panel_name:{} panel_mnid:{}",panel_name,panel_mnid);
        if ((mnid > 0) and (panel_mnid != mnid)) continue;
//-----------------------------------------------------------------------------
// execute command for a given panel
//-----------------------------------------------------------------------------
        if (cmd == "print_status")   PanelPrintStatus(cmd_parameter_path);
        if (cmd == "test_command")   {
          //          TestCommand(stn,pln,pnl);
          std::thread t(TEqTracker::TestCommand,stn,pln,pnl);
          t.detach();
        }
      }
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}
