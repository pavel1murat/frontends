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
std::string OdbInterface::GetString(HNDLE hDB, HNDLE hDir, const char* Key) {
  std::string res;
  char        val[1000];

  int         sz = sizeof(val);
  if (db_get_value(hDB, hDir, Key, &val, &sz, TID_STRING, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "cant find hDB=" << hDB << " hDir=" << hDir << " Key=" << Key ;
  }
  else {
    res = val;
  }

  return res;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetInteger(HNDLE hDB, HNDLE hCFO, const char* Key, int* Data) {
  int   sz = sizeof(*Data);
  int rc = db_get_value(hDB, hCFO, Key, Data, &sz, TID_INT, FALSE);
  TLOG(TLVL_INFO) << "key:" << Key << " value:" << *Data;
  return rc;
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetActiveRunConfig(HNDLE hDB) {
  return GetString(hDB,0,"/Mu2e/ActiveRunConfiguration");
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
  if (db_get_value(hDB, hCFO, "NEwmsPerSecond", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO , return 0 (disabled)";
    return -1;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCFOExternal(HNDLE hDB, HNDLE hCFO) {
  int data(-1);
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "External", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO Type, return type = -1";
  }
  return data;
}

//-----------------------------------------------------------------------------
// emulated CFO: sleep time for rate throttling
//-----------------------------------------------------------------------------
int OdbInterface::GetCFOSleepTime(HNDLE hDB, HNDLE hCFO) {
  const char* key {"SleepTimeMs"};
  int   data(-1);

  if (GetInteger(hDB,hCFO,"SleepTimeMs",&data) != DB_SUCCESS) {
    // data = -1;
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcEmulatesCfo(HNDLE hDB, HNDLE hDTC) {
  const char* key {"EmulatesCFO"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcEnabled(HNDLE hDB, HNDLE hDTC) {
  const char* key {"Enabled"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcLinkMask(HNDLE hDB, HNDLE hDTC) {
  const char* key {"LinkMask"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
int OdbInterface::GetDtcReadoutMode(HNDLE hDB, HNDLE hDTC) {
  const char* key {"ReadoutMode"};
  INT   data(-1);       // if not found, want to be meaningless
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return: " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
// assum hDTC is a DTC handle
//-----------------------------------------------------------------------------
int OdbInterface::GetDtcSampleEdgeMode(HNDLE hDB, HNDLE hDTC) {
  const char* key {"SampleEdgeMode"};
  INT   data(-1);       // if not found, want to be meaningless
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return: " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetNDTCs(HNDLE hDB, HNDLE hCFO, int* NDtcs) {
  int   rc(0);
  int   n_timing_chains(8);
  
  int   sz = sizeof(int)*n_timing_chains;
  
  if (db_get_value(hDB, hCFO, "NDtcs", NDtcs, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO Type, return type = -1";
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetNEvents(HNDLE hDB, HNDLE hCFO) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "NEvents", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO , return 0 (disabled)";
    return -1;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetEventWindowSize(HNDLE hDB, HNDLE hCFO) {
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, "EventWindowSize", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO , return 0 (disabled)";
    data = -1;
  }
  return data;
}


//-----------------------------------------------------------------------------
int OdbInterface::GetDaqHostHandle(HNDLE hDB, HNDLE hConf, const std::string& Hostname) {

  char key[100];
  int  sz = sizeof(key);

  snprintf(key,sz-1,"DetectorConfiguration/DAQ/%s",Hostname.data());

  HNDLE h(0);
  if (db_find_key(hDB, hConf, key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found";
  }
  return h;
}


//-----------------------------------------------------------------------------
std::string OdbInterface::GetCFORunPlanDir(HNDLE hDB) {
  return GetString(hDB,0,"/Mu2e/RunPlanDir");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetCFORunPlan(HNDLE hDB, HNDLE hCFO) {
  return GetString(hDB,hCFO,"RunPlan");
}

//-----------------------------------------------------------------------------
int OdbInterface::GetPcieAddress(HNDLE hDB, HNDLE hCFO) {
  const char* key {"PCIEAddress"};
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(hDB, hCFO, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no CFO PCIE address, return -1";
  }
  return data;
}
