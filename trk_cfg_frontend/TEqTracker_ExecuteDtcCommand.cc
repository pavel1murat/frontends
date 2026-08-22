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
// H_Cmd - handle to a tracker command
//-----------------------------------------------------------------------------
int TEqTracker::ExecuteDtcCommand(HNDLE H_Cmd) { // const std::string& Cmd) {
  int rc(0);

  HNDLE h_panel(0);
                                        // the output of this command ends up split into multiple DTC logfiles - do they?
  
  std::string cmd     = _odb_i->GetString (H_Cmd,"Name"   );

  // for print_status, the command names are different - the tracker has its own 'print_status'
  if (cmd == "print_dtc_status") cmd = "print_status";

  
  std::string logfile = _odb_i->GetString (H_Cmd,"logfile");
  int station         = _odb_i->GetInteger(H_Cmd,"station");
  int plane           = _odb_i->GetInteger(H_Cmd,"plane"  );
  int mnid            = _odb_i->GetInteger(H_Cmd,"mnid"   );
  
  TLOG(TLVL_DEBUG) << std::format("-- START cmd:{} station:{} plane:{} mnid:{}",cmd,station,plane,mnid);

  int first_station(-1), last_station(-1), first_plane(0), last_plane(2);

  if (mnid >= 0) {
                                        // single panel
    int hash = (mnid/10)*10;
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

  TLOG(TLVL_DEBUG) << std::format("first_station:{} last_station:{} first_plane:{} last_plane:{}",
                                  first_station, last_station, first_plane, last_plane);

  std::string cmd_parameter_path = _odb_i->GetTrackerCmdParameterPath(cmd);
//-----------------------------------------------------------------------------
// loop over all active DTCs and execute 'PULSER_ON' etc
// it might make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int is=first_station; is<last_station+1; ++is) {
    HNDLE h_station = _odb_i->GetTrackerStationHandle(is);
    TLOG(TLVL_DEBUG+1) << std::format("  station is:{} h_station:{} enabled:{}",is,h_station,_odb_i->GetEnabled(h_station));
    if (_odb_i->GetEnabled(h_station) == 0) continue;
    for (int pln=first_plane; pln<last_plane; ++pln) {
      HNDLE h_plane = _odb_i->GetTrackerPlaneHandle(is,pln);
      TLOG(TLVL_DEBUG+1) << std::format("   plane pln:{} h_station:{} enabled:{}",pln,h_plane,_odb_i->GetEnabled(h_plane));
      if (_odb_i->GetEnabled(h_plane) == 0) continue;
//-----------------------------------------------------------------------------
// at this point, instead of looping over the panels, need to find the DTC
// and pass parameters to it with link=-1
// [dangerous] assumption that we have a DTC per plane, so everything is simple
//-----------------------------------------------------------------------------
      HNDLE       h_dtc     = _odb_i->GetHandle(h_plane,"DTC");
      int         pcie_addr = _odb_i->GetDtcPcieAddress(h_dtc);
      std::string node      = _odb_i->GetDtcHostLabel  (h_dtc);
//-----------------------------------------------------------------------------
// pass address of parameters stored in the tracker command tree
//-----------------------------------------------------------------------------
      HNDLE       h_dtc_cmd = _odb_i->GetDtcCmdHandle(node,pcie_addr);

      int lnk = -1;
      if (mnid >= 0) {
        lnk = _odb_i->GetInteger(h_panel,"Link");
        TLOG(TLVL_DEBUG+1) << std::format("   link lnk:{} h_panel:{} enabled:{}",pln,h_panel,_odb_i->GetEnabled(h_panel));
        if (_odb_i->GetEnabled(h_panel) == 0) continue;
      }

      TLOG(TLVL_DEBUG+1) << std::format("node:{} pcie_addr:{} link:{}",node,pcie_addr,lnk);

      int dtc_status = _odb_i->GetInteger(h_dtc,"Status");
      if (dtc_status == 0) {
        _odb_i->SetString (h_dtc_cmd,"Name"         ,cmd);
        // set logfile to "default" for the DTC to write in its own logfile
        _odb_i->SetString (h_dtc_cmd,"logfile"      ,"default");
        _odb_i->SetInteger(h_dtc_cmd,"link"         ,lnk);
        _odb_i->SetString (h_dtc_cmd,"ParameterPath",cmd_parameter_path);
        _odb_i->SetInteger(h_dtc_cmd,"Finished"     , 0);
        _odb_i->SetInteger(h_dtc_cmd,"ReturnCode"   , 0);
//-----------------------------------------------------------------------------
// and trigger the execution of a 'per-DTC' command
//-----------------------------------------------------------------------------
        _odb_i->SetInteger(h_dtc_cmd,"Run"          , 1);
      }
      else {
//-----------------------------------------------------------------------------
// dont know what to do - unknown value of 'Finished' - generate diagnostics and
// move on to processing the next panel
//-----------------------------------------------------------------------------
        TLOG(TLVL_ERROR) << std::format("DTC status:{}",dtc_status);
        rc += -1;
      }
    }
  }

  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}
