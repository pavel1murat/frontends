//

#include "TRACE/trace.h"
#include "OdbInterface.hh"

OdbInterface* OdbInterface::_instance(nullptr);


//-----------------------------------------------------------------------------
OdbInterface* OdbInterface::Instance(HNDLE Hdb) {
  if (_instance == nullptr) {
    _instance = new OdbInterface();
  }
  return _instance;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetActiveRunConfig(HNDLE hDB, std::string& RunConf) {
  char run_conf[100];
  int   sz = sizeof(run_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &run_conf, &sz, TID_STRING, FALSE);
  RunConf = run_conf;
  return 0;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetRunConfigHandle(HNDLE hDB, std::string& RunConf) {
  char     key[200];
  HNDLE    h;
  sprintf(key,"/Mu2e/RunConfigurations/%s",RunConf.data());
	if (db_find_key(hDB, 0, key, &h) == DB_SUCCESS) return h;
  else {
    TLOG(TLVL_ERROR) << "no handle for:" << key << ", got handle=" << h;
    return 0;
  }
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetCFOConfigHandle(HNDLE h_DB, HNDLE h_RunConf) {
  const char* key {"DetectorConfiguration/DAQ/CFO"};
  HNDLE    h;
	if (db_find_key(h_DB, h_RunConf, key, &h) == DB_SUCCESS) return h;
  else {
    TLOG(TLVL_ERROR) << "no handle for:" << key << ", got handle=" << h;
    return 0;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetPcieAddress(HNDLE hDB, HNDLE hCFO) {
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "PCIEAddress", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO PCIE address, return -1";
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCFOEnabled(HNDLE hDB, HNDLE hCFO) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "Enabled", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO Enabled, return 0 (disabled)";
    return 0;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCFONEwmPerSecond(HNDLE hDB, HNDLE hCFO) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "NEwmPerSecond", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO , return 0 (disabled)";
    return 0;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCFOType(HNDLE hDB, HNDLE hCFO) {
  int data(-1);
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "Type", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO Type, return type = -1";
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDaqHostHandle(HNDLE hDB, HNDLE hConf, const std::string& Hostname) {

  char key[100];
  int  sz = sizeof(key);

  snprintf(key,sz-1,"DAQ/%s",Hostname.data());

  HNDLE h(0);
  if (db_find_key(hDB, hConf, key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "cant find Host handle for " << Hostname;
  }
  return h;
}
