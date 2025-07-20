///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <format>

// #include "odbxx.h"

#include <iostream>
#include <fstream>

#include "frontends/cfg_frontend/TEqTracker.hh"
#include "utils/utils.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqTracker"

//-----------------------------------------------------------------------------
// "/Mu2e/Commands/Tracker/TRK/
TMFeResult TEqTracker::ResetStationLV(const std::string& CmdParameterPath) {
  std::string tracker_config_path{"/Mu2e/ActiveRunConfiguration/Tracker"};
  std::string tracker_cmd_path   {"/Mu2e/Commands/Tracker"};
  std::string cmd                {"reset_station_lv"};

  TLOG(TLVL_DEBUG) << "--- START"; 

  OdbInterface* odb_i = OdbInterface::Instance();
  
  HNDLE h_cmd     = odb_i->GetHandle(0,tracker_cmd_path);

  HNDLE h_cmd_par = odb_i->GetHandle(0,CmdParameterPath);
  int station     = odb_i->GetInteger(h_cmd_par,"station");
//-----------------------------------------------------------------------------
// figure out the station RPI and passh the command to the RPI
//-----------------------------------------------------------------------------
  std::string station_config_path = std::format("{}/Station_{:02d}",tracker_config_path,station);
  HNDLE       h_station    = odb_i->GetHandle(0,station_config_path);
  std::string rpi_name     = odb_i->GetString(h_station,"RPI/Name");

  std::string rpi_cmd_path = std::format("/Mu2e/Commands/Tracker/RPI/{}",rpi_name);
  HNDLE h_rpi_cmd = odb_i->GetHandle(0,rpi_cmd_path);

  std::string rpi_cmd_par_path = std::format("{}/{}",rpi_cmd_path,cmd);
  HNDLE h_rpi_cmd_par = odb_i->GetHandle(0,rpi_cmd_par_path);

  TLOG(TLVL_DEBUG) << "rpi_cmd_path:"      << rpi_cmd_path
                   << " h_rpi_cmd:"        << h_rpi_cmd
                   << " rpi_cmd_par_path:" << rpi_cmd_par_path
                   << " h_rpi_cmd_par:"    << h_rpi_cmd_par; 

  odb_i->SetInteger(h_rpi_cmd_par,"print_level",1);

  odb_i->SetString (h_rpi_cmd,"Name"         ,cmd);
  odb_i->SetString (h_rpi_cmd,"ParameterPath",rpi_cmd_par_path);
  odb_i->SetInteger(h_rpi_cmd,"Finished"     ,0);
  odb_i->SetInteger(h_rpi_cmd,"Run"          ,1);
//-----------------------------------------------------------------------------
// wait till command completes
//-----------------------------------------------------------------------------
  int max_wait(10000), wait(0), finished(0);
  
  while ((finished == 0) and (wait < max_wait)) {
    finished = odb_i->GetInteger(h_rpi_cmd,"Finished");
    ss_sleep(100);
    wait    += 100;
  }

  if (finished == 0) {
    cm_msg(MERROR, __FILE__,"TIMEOUT");
    TLOG(TLVL_ERROR) << "Timeout"; 
  }
//-----------------------------------------------------------------------------
// the execution is complete, '/Finished' and '/ReturnCode' tell the story
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_cmd,"Run",0);
  
  TLOG(TLVL_DEBUG) << "--- END"; 

  return TMFeOk();
}
