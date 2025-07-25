///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include <iostream>
#include <fstream>

#include "frontends/cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
int TEqTracker::PanelPrintStatus(const std::string& CmdParameterPath) {
  int rc(0);
  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();
  
  TLOG(TLVL_DEBUG) << "cmd_parameter_path:" << CmdParameterPath;
//-----------------------------------------------------------------------------
// find the panel
//-----------------------------------------------------------------------------
  HNDLE h_params = odb_i->GetHandle(0,CmdParameterPath);
  int   mnid     = odb_i->GetInteger(h_params,"mnid"); // o_params["mnid"];
  HNDLE h_panel  = odb_i->GetTrackerPanelHandle(mnid);
//-----------------------------------------------------------------------------
// the name should be constructed based on the experiment name
//-----------------------------------------------------------------------------
  std::vector<std::string> vs;

  uint16_t ch_mask  [96], thr_hv[96], thr_cal[96], gain_hv[96], gain_cal[96];
  float    thr_hv_mv[96], thr_cal_mv[96];
  
  odb_i->GetArray(h_panel,"ch_mask"   ,TID_WORD , ch_mask   ,96);
  odb_i->GetArray(h_panel,"thr_hv"    ,TID_WORD , thr_hv    ,96);
  odb_i->GetArray(h_panel,"thr_cal"   ,TID_WORD , thr_cal   ,96);
  odb_i->GetArray(h_panel,"gain_hv"   ,TID_WORD , gain_hv   ,96);
  odb_i->GetArray(h_panel,"gain_cal"  ,TID_WORD , gain_cal  ,96);
  odb_i->GetArray(h_panel,"thr_hv_mv" ,TID_FLOAT, thr_hv_mv ,96);
  odb_i->GetArray(h_panel,"thr_cal_mv",TID_FLOAT, thr_cal_mv,96);

  for (int i=0; i<96; i++) {
    std::string s = std::format("{:4d} {:6d} {:8d} {:8d} {:8d} {:8d} {:9.3f} {:9.3f}",
                                i,ch_mask[i],thr_hv[i],thr_cal[i],gain_hv[i],gain_cal[i],thr_hv_mv[i],thr_cal_mv[i]);
    vs.emplace_back(s);
  }

  int ns = vs.size();
//-----------------------------------------------------------------------------
// write output to the log
//-----------------------------------------------------------------------------
  std::ofstream output_file;
  output_file.open("/home/mu2etrk/test_stand/experiments/test_025/tracker.log",std::ios::app);

  for (int i=ns-1; i>=0; i--) {
    output_file << vs[i] << std::endl;
  }
  output_file << " ich  ch_mask  thr_hv   thr_cal gain_hv  gain_cal  thr_hv_mv   thr_cal_mv" << std::endl;
  output_file << "----- panel MN" << std::format("{:03d}",mnid) << " ---------------- " << std::endl;

  output_file.close();
                                        // just playing
  //  cm_msg1(MINFO, "tracker","eklmn","-----------DONE printing");

  ss_sleep(100);

  HNDLE h_cmd = odb_i->GetHandle(0,"/Mu2e/Commands/Tracker");
  odb_i->SetInteger(h_cmd,"Finished",1);
  
  TLOG(TLVL_DEBUG) << "--- END: rc:" << rc; 

  return rc;
}
