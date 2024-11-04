#ifndef __utils_OdbInterface_hh__
#define __utils_OdbInterface_hh__

#include "midas.h"

//-----------------------------------------------------------------------------
class OdbInterface {
public:
  HNDLE         _hDB;   // cached
  static OdbInterface* _instance;              // 

private:

  OdbInterface(HNDLE h_DB = 0) { _hDB = h_DB; }

public:

  static OdbInterface*  Instance  (HNDLE h_DB);

  std::string GetActiveRunConfig  (HNDLE h_DB);
  int         GetArtdaqPartition  (HNDLE h_DB);

  HNDLE       GetRunConfigHandle   (HNDLE h_DB, std::string& RunConf);
  HNDLE       GetArtdaqConfigHandle(HNDLE h_DB, std::string& RunConf, std::string& Host);

  HNDLE       GetCFOConfigHandle  (HNDLE h_DB, HNDLE h_RunConf);
  int         GetCFOEnabled       (HNDLE h_DB, HNDLE h_CFO);
  int         GetCFONEwmPerSecond (HNDLE h_DB, HNDLE h_CFO);
  std::string GetCFORunPlanDir    (HNDLE h_DB);
  std::string GetCFORunPlan       (HNDLE h_DB, HNDLE h_CFO);

  int         GetCFOSleepTime     (HNDLE h_DB, HNDLE h_CFO);
  
  int         GetDtcEnabled       (HNDLE h_DB, HNDLE h_Card); // card: DTC
  int         GetDtcEmulatesCfo   (HNDLE h_DB, HNDLE h_Card); // card: DTC
  int         GetDtcLinkMask      (HNDLE h_DB, HNDLE h_Card); // card: DTC
  int         GetDtcReadoutMode   (HNDLE h_DB, HNDLE h_Card); // 0:patterns, 1:data
  int         GetDtcSampleEdgeMode(HNDLE h_DB, HNDLE h_Card); // 0:force rising; 1: force falling; 2:auto
  int         GetDtcPcieAddress   (HNDLE h_DB, HNDLE h_Node); // h_Node: has a "DTC" link to the DTC record
  int         GetPcieAddress      (HNDLE h_DB, HNDLE h_Card); // card: either CFO or DTC
//-----------------------------------------------------------------------------
// CFO supports up to 8 timing chains, so dimension of NDTCs array should be at least 8
// NDTCs[i] : number of DTCs in i-th timing chain
//-----------------------------------------------------------------------------
  int         GetNDTCs            (HNDLE h_DB, HNDLE h_CFO, int* NDTCs);
  
  uint64_t    GetNEvents          (HNDLE h_DB, HNDLE h_DTC);
  int         GetEWLength         (HNDLE h_DB, HNDLE h_DTC);
  uint64_t    GetFirstEWTag       (HNDLE h_DB, HNDLE h_DTC);

                                        // hostname - short, w/o the domain name
  int         GetDaqHostHandle    (HNDLE Hdb, HNDLE h_CFO, const std::string& Hostname);

                                        // returns: 1="external" or 0="emulated"
  int         GetCFOExternal      (HNDLE Hdb, HNDLE h_CFO);

  int         GetInteger          (HNDLE hDB, HNDLE hCFO, const char* Key, int* Data);
  std::string GetString           (HNDLE hDB, HNDLE hDir, const char* Key);
};

#endif
