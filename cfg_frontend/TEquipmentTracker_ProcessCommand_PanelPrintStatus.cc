///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include <iostream>
#include <fstream>

#include "frontends/cfg_frontend/TEquipmentTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentTracker_ProcessCommand_PrintPanelStatus"

//-----------------------------------------------------------------------------
void TEquipmentTracker::ProcessCommand_PanelPrintStatus(const std::string& CmdParameterPath) {
  TLOG(TLVL_DEBUG) << "--- START"; 

  std::string tracker_config_path = "/Mu2e/ActiveRunConfiguration/Tracker";
  midas::odb o_tracker_config(tracker_config_path);

  TLOG(TLVL_DEBUG) << "cmd_parameter_path:" << CmdParameterPath;
//-----------------------------------------------------------------------------
// find the panel
//-----------------------------------------------------------------------------
  midas::odb o_params(CmdParameterPath);

  int mnid = o_params["panel_print_status"]["mnid"];

  int sdir = (mnid/10)*10;
  std::string panel_path = std::format("/Mu2e/Subsystems/Tracker/PanelMap/{:03d}/MN{:03d}/Panel",sdir,mnid);

  TLOG(TLVL_DEBUG) << " panel_path:" << panel_path;

  midas::odb o_panel(panel_path);

  std::ofstream output_file;
  output_file.open("/home/mu2etrk/test_stand/experiments/test_025/junk.log",std::ios::app);

  std::vector<std::string> vs;

  for (int i=0; i<96; i++) {
    int   ch_mask    = o_panel["ch_mask"      ][i];
    int   thr_hv     = o_panel["threshold_hv" ][i];
    int   thr_cal    = o_panel["threshold_cal"][i];
    int   gain_hv    = o_panel["gain_hv"      ][i];
    int   gain_cal   = o_panel["gain_cal"     ][i];
    float thr_hv_mv  = o_panel["thr_hv_mv"    ][i];
    float thr_cal_mv = o_panel["thr_cal_mv"   ][i];

    std::string s = std::format("{:4d} {:6d} {:8d} {:8d} {:8d} {:8d} {:9.3f} {:9.3f}",
                                i,ch_mask,thr_hv,thr_cal,gain_hv,gain_cal,thr_hv_mv,thr_cal_mv);
    vs.emplace_back(s);
  }

  int ns = vs.size();
  for (int i=ns-1; i>=0; i--) {
    output_file << vs[i] << std::endl;
  }
  output_file << " ich  ch_mask  thr_hv   thr_cal gain_hv  gain_cal  thr_hv_mv   thr_cal_mv" << std::endl;
  output_file << "----- panel MN" << std::format("{:03d}",mnid) << " ---------------- " << std::endl;

  output_file.close();
                                        // just playing
  //  cm_msg1(MINFO, "junk","eklmn","-----------DONE printing");

  ss_sleep(100);

  midas::odb o_cmd("/Mu2e/Commands/Tracker");
  o_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "--- END"; 

}
