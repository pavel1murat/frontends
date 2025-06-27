/////////////////////////////////////////////////////////////////////////////
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

using namespace std;
using nlohmann::json;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
// init ODB structure no matter what
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::InitDisk() {
  // char cbuf[100];

  TLOG(TLVL_DEBUG) << "-- START";

//-----------------------------------------------------------------------------
// read ARTDAQ configuration from ODB
//-----------------------------------------------------------------------------
  // HNDLE hdb           = _odb_i->GetDbHandle();
  // HNDLE h_artdaq_conf = _odb_i->GetHostArtdaqConfHandle(_h_active_run_conf,_host_label);

  _prev_ctime_sec = std::time(nullptr);
  _prev_fsize_gb  = 0.;

  InitDiskVarNames();
  
  TLOG(TLVL_DEBUG) << "-- END";
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// disk variables: last_file size, rate to disk, free disk space in the data partition
//-----------------------------------------------------------------------------
void TEquipmentNode::InitDiskVarNames() {
  char dirname[128]; // , name[128];

  TLOG(TLVL_DEBUG) << "-- START";
  const std::string node_odb_path = "/Equipment/"+TMFeEquipment::fEqName;
  const std::string settings_path = node_odb_path+"/Settings";
  midas::odb        odb_settings(settings_path);
//-----------------------------------------------------------------------------
// save in ODB
//-----------------------------------------------------------------------------
  std::vector<std::string> var_names;
  var_names.push_back("ctime");
  var_names.push_back("fsize_gb");
  var_names.push_back("rate_to_disk");
  var_names.push_back("space_used");
  var_names.push_back("space_avail");
  for (int i=0; i<15; i++) var_names.push_back("reserved");
  
  sprintf(dirname,"Names %s","disk");
  char path[256];
  sprintf(path,"%s/%s",settings_path.data(),dirname);
  if (not midas::odb::exists(path)) {
    odb_settings[dirname] = var_names;
  }
  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
int TEquipmentNode::ReadDiskMetrics() {
  int rc(0);
  TLOG(TLVL_DEBUG+1) << "-- START";
  
  std::string cmd = std::format("python config/scripts/monitor_host.py --job=disk_io --dir={}",getenv("DAQ_OUTPUT_TOP"));

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  if (output[0] != '{') {
    TLOG(TLVL_ERROR) << "wrong output:'" << output << "'";
    return -1;
  }

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  json md = json::parse(output);  // 'md' : 'monitoring data'

  TLOG(TLVL_DEBUG+1) << "parsed json:" << md;

  std::time_t ctime_sec = std::time(nullptr); // md["ctime_sec"  ];
  float       fsize_gb  = md["fsize_gb"   ];
  // MBytes/sec
  float rate_to_disk = (fsize_gb - _prev_fsize_gb)/(ctime_sec-_prev_ctime_sec+1.e-12)*1024;

  TLOG(TLVL_DEBUG+1) << "ctime_sec:" << ctime_sec
                     << " _prev_ctime_sec:" << _prev_ctime_sec
                     << " fsize_gb:" << fsize_gb
                     << " _prev_fsize_gb:" << _prev_fsize_gb;

  _prev_ctime_sec = ctime_sec;
  _prev_fsize_gb  = fsize_gb;
//-----------------------------------------------------------------------------
// create bank
//-----------------------------------------------------------------------------
  char buf[1024];
  
  ComposeEvent(buf, sizeof(buf));
  BkInit      (buf, sizeof(buf));
  float* ptr = (float*) BkOpen(buf, "disk", TID_FLOAT);

  *ptr++ = ctime_sec;
  *ptr++ = fsize_gb;
  *ptr++ = rate_to_disk;
  *ptr++ = md["space_used" ];
  *ptr++ = md["space_avail"];
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
  *ptr++ = -1.;
  *ptr++ = -1.;

  TLOG(TLVL_DEBUG+1) << "repacking 1";
//-----------------------------------------------------------------------------
// record has a fixed length !
//-----------------------------------------------------------------------------
  BkClose    (buf,ptr);
  EqSendEvent(buf);
//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG+1) << "-- END, rc:" << rc;
 
  return rc;
}
