///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <format>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"

#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

using namespace std;
using nlohmann::json;

#include "node_frontend/TEqDisk.hh"
#include "utils/TEquipmentManager.hh"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqDisk"

//-----------------------------------------------------------------------------
TEqDisk::TEqDisk(const char* Name, const char* Title) : TMu2eEqBase(Name,Title,TMu2eEqBase::kDisk) {

  _handle                      = _odb_i->GetDiskConfHandle(_h_active_run_conf,_host_label);
   std::string logger_data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  _logfile                     = std::format("{}hosts.log",logger_data_dir);
  _monitoringLevel             = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Disk");

  _data_dir                    = expand_env_vars(_odb_i->GetString(_h_daq_host_conf,"Disk/data_dir"));
  _log_dir                     = expand_env_vars(_odb_i->GetString(_h_daq_host_conf,"Disk/log_dir"));
  
  _prev_ctime_sec              = std::time(nullptr);
  _prev_fsize_gb               = 0.;

  int rc = InitVarNames();
}

//-----------------------------------------------------------------------------
TEqDisk::~TEqDisk() {
}

//-----------------------------------------------------------------------------
TMFeResult TEqDisk::Init() {
  int rc(0);
  TLOG(TLVL_DEBUG) << "-- START";
  TLOG(TLVL_DEBUG) << std::format("-- END: rc:{}",rc);
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// boardreaders: 'br01', 'br02' , etc
//-----------------------------------------------------------------------------
int TEqDisk::InitVarNames() {
  char dirname[128]; // , name[128];

  TLOG(TLVL_DEBUG) << "-- START";
  
  const std::string node_odb_path = "/Equipment/"+HostLabel();
  const std::string settings_path = node_odb_path+"/Settings";
  midas::odb        odb_settings(settings_path);
//-----------------------------------------------------------------------------
// save in ODB
//-----------------------------------------------------------------------------
  std::vector<std::string> var_names;
  var_names.push_back("ctime"       );
  var_names.push_back("fsize_gb"    );
  var_names.push_back("rate_to_disk");
  var_names.push_back("data_space_used" );
  var_names.push_back("data_space_avail");
  var_names.push_back("log_space_used" );
  var_names.push_back("log_space_avail");
  
  for (int i=0; i<15; i++) var_names.push_back(std::format("disk_{:02d}",i+5));
  
  sprintf(dirname,"Names %s","disk");
  char path[256];
  sprintf(path,"%s/%s",settings_path.data(),dirname);
  if (not midas::odb::exists(path)) {
    odb_settings[dirname] = var_names;
  }

  TLOG(TLVL_DEBUG) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
// bank name should be 4 chars long
//-----------------------------------------------------------------------------
int TEqDisk::HandlePeriodic() {
  int rc(0);

  TLOG(TLVL_DEBUG+1) << "-- START";
  std::string cmd = std::format("python config/scripts/monitor_host.py --job=disk_io");
  
  if (_data_dir != "") cmd += std::format(" --data_dir={}", _data_dir);
  if (_log_dir  != "") cmd += std::format(" --log_dir={}" , _log_dir );

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  if (output[0] != '{') {
    TLOG(TLVL_ERROR) << "wrong output:'" << output << "'";
    return -1;
  }

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  float fsize_gb(-1.), rate_to_disk(-1.), log_space_used(-1.), log_space_avail(-1.), data_space_used(-1.), data_space_avail(-1.);
  std::time_t ctime_sec = std::time(nullptr); // md["ctime_sec"  ];
  
  try {
    json md = json::parse(output);  // 'md' : 'monitoring data'

    TLOG(TLVL_DEBUG+1) << "parsed json:" << md;

    fsize_gb         = md["fsize_gb"        ];
    log_space_avail  = md["log_space_avail" ];
    log_space_used   = md["log_space_used"  ];
    data_space_avail = md["data_space_avail"];
    data_space_used  = md["data_space_used" ];
    
    // MBytes/sec
    rate_to_disk = (fsize_gb - _prev_fsize_gb)/(ctime_sec-_prev_ctime_sec+1.e-12)*1024;

    TLOG(TLVL_DEBUG+1) << "ctime_sec:" << ctime_sec
                       << " _prev_ctime_sec:" << _prev_ctime_sec
                       << " fsize_gb:" << fsize_gb
                       << " _prev_fsize_gb:" << _prev_fsize_gb;
    
    _prev_ctime_sec = ctime_sec;
    _prev_fsize_gb  = fsize_gb;
  }
  catch (...) {
    TLOG(TLVL_ERROR) << std::format("JSON parser failed to parse output");
    rc = -1;
  }
//-----------------------------------------------------------------------------
// create bank
//-----------------------------------------------------------------------------
  char buf[1024];
  
  TEquipmentManager* eqm = TEquipmentManager::Instance();

  eqm->ComposeEvent(buf, sizeof(buf));
  eqm->BkInit      (buf, sizeof(buf));
  float* ptr = (float*) eqm->BkOpen(buf, "disk", TID_FLOAT);

  *ptr++ = ctime_sec;
  *ptr++ = fsize_gb;
  *ptr++ = rate_to_disk;
  *ptr++ = log_space_used;
  *ptr++ = log_space_avail;
  *ptr++ = data_space_used;
  *ptr++ = data_space_avail;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;
  *ptr++ = -1.;

  TLOG(TLVL_DEBUG+1) << "repacking 1";
//-----------------------------------------------------------------------------
// record has a fixed length !
//-----------------------------------------------------------------------------
  eqm->BkClose    (buf,ptr);
  eqm->EqSendEvent(buf);

  TLOG(TLVL_DEBUG+1) << "-- END, rc:" << rc;
 
  return rc;
}
