//

#include "TRACE/trace.h"
#include "OdbInterface.hh"
#include "utils/utils.hh"
#include <format>

OdbInterface* OdbInterface::_instance(nullptr);

//-----------------------------------------------------------------------------
OdbInterface::OdbInterface(HNDLE h_DB) {
  _hDB = h_DB;
}


//-----------------------------------------------------------------------------
// will become the default
//-----------------------------------------------------------------------------
OdbInterface* OdbInterface::Instance() {
  if (_instance == nullptr) {
    HNDLE hDB;
    cm_get_experiment_database(&hDB, NULL);
    _instance = new OdbInterface(hDB);
  }
  return _instance;
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
HNDLE OdbInterface::GetHandle(HNDLE hConf, const std::string& Key) {
  HNDLE h(0);
  
  if (db_find_key(_hDB, hConf, Key.data(), &h) != DB_SUCCESS) {
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
int OdbInterface::GetKey(HNDLE hConf, KEY* Key) {
  return db_get_key(_hDB,hConf,Key);
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetParent(HNDLE hConf) {
  HNDLE h_parent(0);
  if (db_get_parent(_hDB,hConf,&h_parent) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no parent for hConf:" << hConf;
  }
  TLOG(TLVL_DEBUG+1) << "hConfe:" << hConf << " h_parent:" << h_parent;
  return h_parent;
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
int OdbInterface::GetArray(HNDLE hDir, const char* Key, int DataType, void* Data, int NElements) {
  int rc(0), sz(0);

  if      (DataType == TID_INT32 ) sz= sizeof(int32_t );
  else if (DataType == TID_WORD  ) sz= sizeof(uint16_t);
  else if (DataType == TID_UINT32) sz= sizeof(uint32_t);
  else if (DataType == TID_FLOAT ) sz= sizeof(float   );
  else if (DataType == TID_DOUBLE) sz= sizeof(double  );
  else {
    TLOG(TLVL_ERROR) << "not yet implemented data type:" << DataType << " BAIL OUT";
    return -1;
  }
  sz = sz*NElements;
  HNDLE h = GetHandle(hDir,Key);
  rc = db_get_data(_hDB, h, Data, &sz, DataType);
  if (rc != DB_SUCCESS) {
    KEY dbkey;
    db_get_key(_hDB,hDir,&dbkey);
    TLOG(TLVL_ERROR) << "cant find key:" << Key << " in hDir:" << dbkey.name << "(" << hDir << ")";
  }
  TLOG(TLVL_DEBUG+1) << "--END: key:" << Key << " rc:" << rc;

  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::GetInteger(HNDLE hDir, const char* Key) {
  int rc(0), res;
  int sz = sizeof(int);

  rc = db_get_value(_hDB, hDir, Key, &res, &sz, TID_INT, FALSE);
  if (rc != DB_SUCCESS) {
    KEY dbkey;
    db_get_key(_hDB,hDir,&dbkey);
    TLOG(TLVL_ERROR) << "cant find key:" << Key << " in hDir:" << dbkey.name << "(" << hDir << ")";
    res = -9999;
  }
  TLOG(TLVL_DEBUG+1) << "key:" << Key << " value:" << res;

  return res;
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
uint32_t OdbInterface::GetDtcFwVersion(HNDLE h_RunConf) {
  const char* key{"DAQ/DtcFwVersion"};
  
  uint32_t  data(0);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, h_RunConf, key, &data, &sz, TID_UINT32, FALSE) == DB_SUCCESS) {
    return data;
  }
  else {
    TLOG(TLVL_ERROR) << "cant retrieve key:" << key << ", return -1";
    return -1;
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
int OdbInterface::GetLinkEnabled(HNDLE hDTC, int Link) {
  std::string s = std::format("Link{:d}",Link);
  HNDLE h_link = GetHandle(hDTC,s.data());
  return GetEnabled(h_link);
}

//-----------------------------------------------------------------------------
int OdbInterface::SetLinkMask(HNDLE hDTC, int LinkMask) {
  int rc(0);
  
  HNDLE h = GetHandle(hDTC,"LinkMask");
  if (db_set_data(_hDB,h,(void*) &LinkMask,sizeof(int),1,TID_UINT32) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set LinkMask:" << "0x" << std::hex << LinkMask;
    rc = -1;
  }
  return rc;
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
int OdbInterface::GetSubsystem(HNDLE hDTC) {
  const char* key {"Subsystem"};
  INT   data(-1);      // if not found, set to undefined
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
// hRunConf = -1: request for active run configuration
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetDaqConfigHandle(HNDLE hRunConf) {
  const char* key {"DAQ"};

  HNDLE h(0), h_conf(hRunConf);
  
  if (hRunConf == -1) h_conf = GetHandle(0,"/Mu2e/ActiveRunConfiguration");

  if (db_find_key(_hDB, h_conf, key, &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found";
  }
  return h;
}

//-----------------------------------------------------------------------------
// hRunConf = -1: request for active run configuration
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetDtcConfigHandle(const std::string& Host, int PcieAddr, HNDLE hRunConf) {
  HNDLE h(0), h_conf(hRunConf);
  
  if (hRunConf == -1) h_conf = GetHandle(0,"/Mu2e/ActiveRunConfiguration");

  std::string key = std::format("DAQ/Nodes/{}/DTC{}",Host,PcieAddr);

  if (db_find_key(_hDB, h_conf, key.data(), &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << key << " not found";
  }
  return h;
}

//-----------------------------------------------------------------------------
// hRunConf = -1: request for active run configuration
// 'Host' is the host label (w/o '-ctrl')
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetDtcCommandHandle(const std::string& Host, int PcieAddr) {
  HNDLE h(0);
  
  std::string key = std::format("/Mu2e/Commands/DAQ/Nodes/{}/DTC{}",Host,PcieAddr);

  if (db_find_key(_hDB, 0, key.data(), &h) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "DTC command handle for host:" << Host
                     << " and PcieAddr:" << PcieAddr << " not found";
  }
  return h;
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetFrontendConfHandle(HNDLE hRunConf, const std::string& Host) {
  char key[128];
  sprintf(key,"DAQ/Nodes/%s/Frontend",Host.data());
  return GetHandle(_hDB,hRunConf,key);
}

//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetHostConfHandle(const std::string& Host, HNDLE hRunConf) {
  HNDLE h_conf(hRunConf);
  if (h_conf == -1) h_conf = GetHandle(0,"/Mu2e/ActiveRunConfiguration");
  
  std::string key = "DAQ/Nodes/"+Host;
  return GetHandle(_hDB,h_conf,key.data());
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
int OdbInterface::GetPcieAddress(HNDLE hCard) {
  const char* key {"PCIEAddress"};
  INT   data(-1);
  int   sz = sizeof(data);
  if (db_get_value(_hDB, hCard, key, &data, &sz, TID_INT, FALSE) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no PCIE address, return -1";
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
// need to substitute the env vars
//-----------------------------------------------------------------------------
std::string OdbInterface::GetConfigDir() {
  std::string s = GetString(0,"Mu2e/ConfigDir");
  return expand_env_vars(s);
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetOutputDir() {
  std::string s = GetString(0,"Mu2e/OutputDir");
  return expand_env_vars(s);
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
int OdbInterface::SetRocID(HNDLE hLink, std::string& RocID) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"DetectorElement/RocDeviceSerial");
  if (db_set_data(_hDB,h,(void*) RocID.data(),strlen(RocID.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set ROC ID:" << RocID;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::SetRocDesignInfo(HNDLE hLink, std::string& DesignInfo) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"DetectorElement/RocDesignInfo");
  if (db_set_data(_hDB,h,(void*) DesignInfo.data(),strlen(DesignInfo.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set design info:" << DesignInfo;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
int OdbInterface::SetRocFwGitCommit(HNDLE hLink, std::string& Commit) {
  int rc(0);
  HNDLE h = GetHandle(hLink,"DetectorElement/RocGitCommit");
  if (db_set_data(_hDB,h,(void*) Commit.data(),strlen(Commit.data())+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set git commit:" << Commit;
    rc = -1;
  }
  return rc;
}

//-----------------------------------------------------------------------------
// commands
//-----------------------------------------------------------------------------
HNDLE OdbInterface::GetCommandHandle(const std::string& Subsystem) {
  std::string path = "/Mu2e/Commands/"+Subsystem;
  return GetHandle(0,path);
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCommand_Run(HNDLE h_Cmd) {
  return GetInteger(h_Cmd,"Run");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetCommand_Name(HNDLE h_Cmd) {
  return GetString(h_Cmd,"Name");
}

//-----------------------------------------------------------------------------
std::string OdbInterface::GetCommand_ParameterPath(HNDLE h_Cmd) {
  return GetString(h_Cmd,"ParameterPath");
}

//-----------------------------------------------------------------------------
int OdbInterface::GetCommand_Finished(HNDLE h_Cmd) {
  return GetInteger(h_Cmd,"Finished");
}

//-----------------------------------------------------------------------------
int OdbInterface::SetArray(HNDLE hDir, const char* Key, int DataType, void* Data, int NElements) {
  int rc(0), sz(0);

  if      (DataType == TID_INT32 ) sz= sizeof(int32_t );
  else if (DataType == TID_WORD  ) sz= sizeof(uint16_t);
  else if (DataType == TID_UINT32) sz= sizeof(uint32_t);
  else if (DataType == TID_FLOAT ) sz= sizeof(float   );
  else if (DataType == TID_DOUBLE) sz= sizeof(double  );
  else {
    TLOG(TLVL_ERROR) << "not yet implemented data type:" << DataType << " BAIL OUT";
    return -1;
  }
  int tot_sz = sz*NElements;
  HNDLE h = GetHandle(hDir,Key);
  rc = db_set_data(_hDB, h, Data, tot_sz, NElements, DataType);
  if (rc != DB_SUCCESS) {
    KEY dbkey;
    db_get_key(_hDB,hDir,&dbkey);
    TLOG(TLVL_ERROR) << "cant find key:" << Key << " in hDir:" << dbkey.name << "(" << hDir << ")";
  }
  TLOG(TLVL_DEBUG+1) << "--END: key:" << Key << " rc:" << rc;

  return rc;
}

//-----------------------------------------------------------------------------
void OdbInterface::SetInteger(HNDLE hElement, const char* Key, int Value) {
  HNDLE h = GetHandle(hElement,Key);
  if (db_set_data(_hDB,h,(void*) &Value,sizeof(int),1,TID_INT32) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set Key:" << Key << " to : " << Value;
  }
}

//-----------------------------------------------------------------------------
void OdbInterface::SetString(HNDLE hElement, const char* Key, const std::string& Value) {
  HNDLE h = GetHandle(hElement,Key);
  
  if (db_set_data(_hDB,h,(void*) Value.data(),Value.length()+1,1,TID_STRING) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to set Key:" << Key << " to : " << Value;
  }
}

//-----------------------------------------------------------------------------
void OdbInterface::SetStatus(HNDLE hElement, int Status) {
  SetInteger(hElement,"Status",Status);
}

//-----------------------------------------------------------------------------
void OdbInterface::SetCommand_Finished(HNDLE h_Cmd, int Value) {
  SetInteger(h_Cmd,"Finished",Value);
}

//-----------------------------------------------------------------------------
// tracker section
//-----------------------------------------------------------------------------

HNDLE OdbInterface::GetTrackerPanelHandle(int MnID) {
  int sdir = (MnID/10)*10;
  std::string panel_path = std::format("/Mu2e/ActiveRunConfiguration/Tracker/PanelMap/{:03d}/MN{:03d}/Panel",sdir,MnID);

  HNDLE h = GetHandle(0,panel_path);
  return h;
}
