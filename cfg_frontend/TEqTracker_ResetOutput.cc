///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

#include "odbxx.h"

#include <iostream>
#include <fstream>

#include "frontends/cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
void TEqTracker::ResetOutput() {
  TLOG(TLVL_DEBUG) << "--- START"; 

  std::ofstream output_file;
  output_file.open("/home/mu2etrk/test_stand/experiments/test_025/tracker.log",std::ofstream::trunc);
  output_file.close();
                                        // just playing
  ss_sleep(100);

  midas::odb o_cmd("/Mu2e/Commands/Tracker");
  o_cmd["Finished"] = 1;
  
  TLOG(TLVL_DEBUG) << "--- END"; 

}
