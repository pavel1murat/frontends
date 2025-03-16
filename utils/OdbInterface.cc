//

#include "TRACE/trace.h"
#include "OdbInterface.hh"


OdbInterface* OdbInterface::_instance(nullptr);

//-----------------------------------------------------------------------------
OdbInterface::OdbInterface(HNDLE h_DB) {
  _hDB = h_DB;
}


//-----------------------------------------------------------------------------
OdbInterface* OdbInterface::Instance(HNDLE Hdb) {
  if (_instance == nullptr) {
    _instance = new OdbInterface(Hdb);
  }
  else if (_instance->_hDB != Hdb) {
    _instance->_hDB = Hdb;
  }
  return _instance;
}


//-----------------------------------------------------------------------------
std::string OdbInterface::GetString(HNDLE hDir, const char* Key) {
  std::string res;
  char        val[1000];

  int         sz = sizeof(val);
  if (db_get_value(_hDB, hDir, Key, &val, &sz, TID_STRING, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "cant find the key ; hDB=" << _hDB << " hDir=" << hDir << " Key=" << Key ;
  }
  else {
    res = val;
  }

  TLOG(TLVL_DEBUG+1) << "key:" << Key << " res:" << res;
  return res;
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetString(HNDLE hDB, HNDLE hDir, const char* Key) {
  return GetString(hDir,Key);
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetHandle(HNDLE hConf, const char* Key) {
  HNDLE h(0);
  
  if (db_find_key(_hDB, hConf, Key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no handle for hConf:Key:" << hConf << ":" << Key;
  }
  TLOG(TLVL_DEBUG+1) << "key:" << Key << " h:" << h;
  return h;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetHandle(HNDLE hDB, HNDLE hConf, const char* Key) {
  return GetHandle(hConf,Key);
}

//-----------------------------------------------------------------------------
int OdbInterface::GetUInt32(HNDLE hDir, const char* Key, uint32_t* Data) {
  int rc(0);
  int sz = sizeof(uint32_t); 
  rc = db_get_value(_hDB, hDir, Key, Data, &sz, TID_UINT32, FALSE);
  if (rc != DB_SUCCESS) {
    KEY dbkey;
    db_get_key(_hDB,hDir,&dbkey);
    TLOG(TLVL_ERROR) << "cant find key:" << Key << " in hDir:" << dbkey.name << "(" << hDir << ")"; 
  }
  TLOG(TLVL_DEBUG+1) << "key:" << Key << " value:" << Data;
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetInteger(HNDLE hDir, const char* Key, int* Data) {
  int rc(0);
  int sz = sizeof(int); 
  rc = db_get_value(_hDB, hDir, Key, Data, &sz, TID_INT, FALSE);
  if (rc != DB_SUCCESS) {
    KEY dbkey;
    db_get_key(_hDB,hDir,&dbkey);
    TLOG(TLVL_ERROR) << "cant find key:" << Key << " in hDir:" << dbkey.name << "(" << hDir << ")"; 
  }
  TLOG(TLVL_DEBUG+1) << "key:" << Key << " value:" << Data;
  return rc;
}


int OdbInterface::GetInteger(HNDLE hDB, HNDLE hDir, const char* Key, int* Data) {
  return GetInteger(hDir,Key,Data);
}

//-----------------------------------------------------------------------------
int OdbInterface::GetEnabled(HNDLE hConf) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hConf, "Enabled", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    KEY dbkey;
    db_get_key(_hDB,hConf,&dbkey);
    TLOG(TLVL_ERROR) << "no " << dbkey.name << ".Enabled field, return 0 (disabled)";
    return 0;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetStatus(HNDLE hConf) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hConf, "Status", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    KEY dbkey;
    db_get_key(_hDB,hConf,&dbkey);
    TLOG(TLVL_ERROR) << "no " << dbkey.name << ".Enabled field, return 0 (disabled)";
    return 0;
  }
}

//-----------------------------------------------------------------------------
// /Mu2e/ActiveRunConfiguration is a link to the active run configuration
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetActiveRunConfigHandle() {
  return GetHandle(_hDB,0,"/Mu2e/ActiveRunConfiguration");
}

//-----------------------------------------------------------------------------
// this could become DAQ/Nodes/mu2edaqXXX
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetHostArtdaqConfHandle(HNDLE h_RunConf, const std::string& Host) {
  char key[128];
  sprintf(key,"DAQ/Nodes/%s/Artdaq",Host.data());
  return GetHandle(_hDB,h_RunConf,key);
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetCfoConfHandle(HNDLE h_RunConf) {
  const char* key {"DAQ/CFO"};
  HNDLE    h(0);
  
  if (db_find_key(_hDB, h_RunConf, key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no handle for:" << key << ", got handle=" << h;
  }
  return h;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCfoEnabled(HNDLE hCFO) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCFO, "Enabled", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO Enabled, return 0 (disabled)";
    return 0;
  }
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCfoEmulatedMode(HNDLE hCFO) {
  INT   data(0);

  GetInteger(_hDB,hCFO,"EmulatedMode",&data);
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCfoEventMode(HNDLE hCFO) {
  INT   data(0);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCFO, "EventMode", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO EventMode, return 0";
    return 0;
  }
}

//-----------------------------------------------------------------------------
// 'SkipDtcInit' flag is defined in ODB at "$run_conf/DAQ/SkipDtcInit",
// the policy is assumed to be the same for all DTCs in the configuration
//-----------------------------------------------------------------------------
int OdbInterface::GetSkipDtcInit(HNDLE h_RunConf) {
  const char* key{"DAQ/SkipDtcInit"};
  
  INT   data(0);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, h_RunConf, key, &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "cant retrieve key:" << key << ", return -1";
    return -1;
  }
}

//-----------------------------------------------------------------------------
// for the emulated CFO
//-----------------------------------------------------------------------------
int OdbInterface::GetCfoNEventsPerTrain(HNDLE hCFO) {
  INT   data;
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCFO, "NEventsPerTrain", &data, &sz, TID_INT, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "no CFO , return 0 (disabled)";
    return -1;
  }
}

//-----------------------------------------------------------------------------
// emulated CFO: sleep time for rate throttling
//-----------------------------------------------------------------------------
int OdbInterface::GetCfoSleepTime(HNDLE hCFO) {
  const char* key {"SleepTimeMs"};
  int   data(-1);

  if (GetInteger(_hDB,hCFO,key,&data) != DB_SUCCESS) {
    // data = -1;
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetRunConfigHandle(std::string& RunConf) {
  char     key[128];
  sprintf(key,"/Mu2e/RunConfigurations/%s",RunConf.data());

  return GetHandle(0,key);
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetRunConfigName(HNDLE hConf) {
  const char* key {"Name"};
  return GetString(_hDB, hConf, key);
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcEmulatesCfo(HNDLE hDTC) {
  const char* key {"EmulatesCFO"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcID(HNDLE hNode) {
  const char* key {"DtcID"};
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hNode, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no DTC ID, return -1";
  }
  return data;
}

//-----------------------------------------------------------------------------
// handle: CFO or DTC record in ODB
//-----------------------------------------------------------------------------
int OdbInterface::GetJAMode(HNDLE hDTC) {
  const char* key {"JAMode"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "JAMode not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetLinkMask(HNDLE hDTC) {
  uint32_t   data(0);       // if not found, want all links to be disabled
  GetUInt32(hDTC,"LinkMask",&data);
  return (int) data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetDtcMacAddrByte(HNDLE hDTC) {
  const char* key {"MacAddrByte"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
// assum hDTC is a DTC handle
//-----------------------------------------------------------------------------
int OdbInterface::GetDtcSampleEdgeMode(HNDLE hDTC) {
  const char* key {"SampleEdgeMode"};
  INT   data(-1);       // if not found, want to be meaningless
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found, return: " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetIsCrv(HNDLE hDTC) {
const char* key {"IsCrv"};
  INT   data(0);       // if not found, set to false
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDTC, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found, return: " << data;
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
uint64_t OdbInterface::GetNEvents(HNDLE hDB, HNDLE hDTC) {
  UINT64   data;
  int   sz = sizeof(data);
  if (db_get_value(hDB, hDTC, "NEvents", &data, &sz, TID_UINT64, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "couldnt find , return 0 (disabled)";
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetEWLength(HNDLE hCFO) {
  INT  data(0);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCFO, "EventWindowSize", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "return 0 (disabled)";
  }
  return data;
}

//-----------------------------------------------------------------------------
// uint64_t OdbInterface::GetFirstEWTag(HNDLE hCFO) {
int OdbInterface::GetFirstEWTag(HNDLE hCFO) {
  int  data(0);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCFO, "FirstEWTag", &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "return 0 (disabled)";
  }
  return data;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetDaqConfigHandle(HNDLE hRunConf) {
  const char* key {"DAQ"};

  HNDLE h(0);
  if (db_find_key(_hDB, hRunConf, key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found";
  }
  return h;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetFrontendConfHandle(HNDLE h_RunConf, const std::string& Host) {
  char key[128];
  sprintf(key,"DAQ/Nodes/%s/Frontend",Host.data());
  return GetHandle(_hDB,h_RunConf,key);
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetHostConfHandle(HNDLE h_RunConf, const std::string& Host) {
  char key[128];
  sprintf(key,"DAQ/Nodes/%s",Host.data());
  return GetHandle(_hDB,h_RunConf,key);
}

//-----------------------------------------------------------------------------
// stored in "/Mu2e/CfoRunPlanDir" may be an environment variable
//-----------------------------------------------------------------------------
std::string OdbInterface::GetCfoRunPlanDir() {
  std::string s = GetString(_hDB,0,"/Mu2e/CfoRunPlanDir");
  if (s[0] == '$') {
    s = getenv(s.substr(1).data());
  }
  return s;
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetCfoRunPlan(HNDLE hCFO) {
  return GetString(_hDB,hCFO,"RunPlan");
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

//-----------------------------------------------------------------------------
// get PCIE address of a boardreader DTC
//-----------------------------------------------------------------------------
int OdbInterface::GetDtcPcieAddress(HNDLE hDtc) {
  const char* key {"PCIEAddress"};
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hDtc, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no DTC PCIE address, return -1";
  }
  return data;
}

//-----------------------------------------------------------------------------
// the ROC readout should be the same for all ROCs in the configuration
// hDetConf - handle of the detector configuration
//-----------------------------------------------------------------------------
int OdbInterface::GetRocReadoutMode(HNDLE hDetConf) {
  int data(-1);
  GetInteger(_hDB,hDetConf,"DAQ/RocReadoutMode",&data);
  return data;
}

int OdbInterface::GetEventMode(HNDLE hDetConf) {
  int data(-1);
  GetInteger(_hDB,hDetConf,"DAQ/EventMode",&data);
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetPartitionID(HNDLE h_RunConf) {
  const char* key {"DAQ/PartitionID"};
  int   data(-1);
  if (GetInteger(h_RunConf,key,&data) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetOnSpill(HNDLE h_RunConf) {
  const char* key {"DAQ/OnSpill"};
  INT   data(0);       // if not found, want all links to be disabled
  int   sz = sizeof(data);
  if (db_get_value(_hDB, h_RunConf, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << "not found, return " << data;
  }
  return data;
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetOutputDir() {
  return GetString(0,"Mu2e/OutputDir");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetPrivateSubnet(HNDLE hRunConf) {
  return GetString(hRunConf,"DAQ/PrivateSubnet");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetPublicSubnet(HNDLE hRunConf) {
  return GetString(hRunConf,"DAQ/PublicSubnet");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetTfmHostName(HNDLE hRunConf) {
  return GetString(hRunConf,"DAQ/Tfm/RpcHost");
}

//-----------------------------------------------------------------------------
int OdbInterface::SetStatus(HNDLE hElement, int Status) {
  int rc(0);
  HNDLE h = GetHandle(hElement,"Status");
  if (db_set_data(_hDB,h,(void*) &Status,sizeof(int),1,TID_INT32) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set status:" << Status;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::SetRocID(HNDLE hLink, std::string& RocID) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"RocDeviceSerial");
  if (db_set_data(_hDB,h,(void*) RocID.data(),strlen(RocID.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set ROC ID:" << RocID;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::SetRocDesignInfo(HNDLE hLink, std::string& DesignInfo) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"RocDesignInfo");
  if (db_set_data(_hDB,h,(void*) DesignInfo.data(),strlen(DesignInfo.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set design info:" << DesignInfo;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::SetRocFwGitCommit(HNDLE hLink, std::string& Commit) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"RocGitCommit");
  if (db_set_data(_hDB,h,(void*) Commit.data(),strlen(Commit.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set git commit:" << Commit;
    rc = -1;
  }
  return rc;
}

