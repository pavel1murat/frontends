///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include "frontends/cfg_frontend/TEquipmentTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentTracker_ProcessCommand_PulserOn"

//-----------------------------------------------------------------------------
void TEquipmentTracker::ProcessCommand_PulserOn(const std::string& CmdParameterPath) {
  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();

  std::string tracker_config_path = "/Mu2e/ActiveRunConfiguration/Tracker";
  midas::odb o_tracker_config(tracker_config_path);
  int first_station = o_tracker_config["FirstStation"];
  int last_station  = o_tracker_config["LastStation" ];

  TLOG(TLVL_DEBUG) << " CmdParameterPath:" << CmdParameterPath;

// loop over all active ROCs and execute 'PULSER_ON'
// it woudl make sense, at initialization stage, to build a list of DTCs assosiated
// with the tracker and execute all DTC commands in a loop over the DTCs, rather than
// looping over the stations... Later
//-----------------------------------------------------------------------------
  for (int is=first_station; is<last_station+1; ++is) {
    std::string station_path = std::format("{:s}/Station_{:02d}",tracker_config_path.data(),is);
    TLOG(TLVL_DEBUG) << " process station:" << is << " station_path:" << station_path;
    midas::odb o_station(station_path);
    if (o_station["Enabled"] == 0) continue;
    for (int pln=0; pln<2; ++pln) {
      std::string plane_path = std::format("{:s}/Plane_{:02d}",station_path.data(),pln);
      TLOG(TLVL_DEBUG) << "          process plane:" << pln << " plane_path:" << plane_path;
      midas::odb o_plane(plane_path);
      if (o_plane["Enabled"] == 0) continue;
      for (int pnl=0; pnl<6; ++pnl) {
        std::string panel_path = std::format("{:s}/Panel_{:02d}",plane_path.data(),pnl);
        TLOG(TLVL_DEBUG) << "          process panel:" << pnl << " panel_path:" << panel_path;
        midas::odb o_panel(panel_path);
        if (o_panel["Enabled"] == 0) continue;
//-----------------------------------------------------------------------------
// pulser_on for this panel: pass parameters to the DTC 
// need to figure the node name, DTC ID, and the link
//-----------------------------------------------------------------------------
        std::string panel_name = o_panel["Name"];
        int mnid = atoi(panel_name.substr(2).data());
        int sdir = (mnid/10)*10;
        std::string panel_map_path = std::format("/Mu2e/Subsystems/Tracker/PanelMap/{:03d}/{:s}",sdir,panel_name.data());

        std::string dtc_path       = std::format("{:s}/DTC",panel_map_path.data());

        TLOG(TLVL_DEBUG) << " dtc_path:" << dtc_path;
        HNDLE h_dtc    = odb_i->GetHandle(0,dtc_path.data());
        HNDLE h_parent = odb_i->GetParent(h_dtc);
          
                                        // node_fe.name is the node frontend name ! 
        KEY node_fe;
        odb_i->GetKey(h_parent,&node_fe);

        midas::odb o_dtc (panel_map_path+"/DTC");

        int pcie_addr = o_dtc  ["PcieAddress"];

        TLOG(TLVL_DEBUG) << "node_fe.name:" << (char*) node_fe.name
                         << " pcie_addr:" << pcie_addr;
        
        midas::odb o_trk_cmd("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on");
        
        std::string dtc_cmd_path = std::format("/Mu2e/Commands/Frontends/{:s}/DTC{:d}",
                                               node_fe.name,pcie_addr);
        midas::odb o_dtc_cmd(dtc_cmd_path);
          
        int link         = o_trk_cmd["link"              ];
        int mask         = o_trk_cmd["first_channel_mask"];
        int duty_cycle   = o_trk_cmd["duty_cycle"        ];
        int pulser_delay = o_trk_cmd["pulser_delay"      ];
        int print_level  = o_trk_cmd["print_level"       ];

        TLOG(TLVL_DEBUG) << "dtc_cmd_path:" << dtc_cmd_path
                         << " link:" << link
                         << " mask:" << mask
                         << " duty_cycle:" << duty_cycle
                         << " pulser_delay:" << pulser_delay
                         << " print_level:" << print_level;
        
        o_dtc_cmd["Name"         ] = "control_roc_pulser_on";
        o_dtc_cmd["ParameterPath"] = CmdParameterPath;
//-----------------------------------------------------------------------------
// finally, execute the command. THe loop is executed fast, so need to wait
// for the DTC's to report that they are finished
//-----------------------------------------------------------------------------
        o_dtc    ["Status"   ] = 1;
        o_dtc_cmd["Finished" ] = 0;
        o_dtc_cmd["Run"      ] = 1;
//-----------------------------------------------------------------------------
// wait till DTC comes back 
//-----------------------------------------------------------------------------
        ss_sleep(100);
        int finished = 0; // simulate .... 0;
        int wait_time(0);
        while ((finished == 0) and (wait_time < 10000)) {
          ss_sleep(100);
          finished = o_dtc_cmd["Finished"];
          wait_time += 100;
        }

        if (finished == 0) {
                                        // show DTC in ERROR
          o_dtc["Status"] = -1;
        }
        else {
          o_dtc["Status"] = 0;
        }
      }
    }
  }
  TLOG(TLVL_DEBUG) << "--- END"; 

}
