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

  int         GetActiveRunConfig  (HNDLE h_DB, std::string& RunConf);

  HNDLE       GetRunConfigHandle  (HNDLE h_DB, std::string& RunConf);

  HNDLE       GetCFOConfigHandle  (HNDLE h_DB, HNDLE h_RunConf);
  int         GetPcieAddress      (HNDLE h_DB, HNDLE h_CFO);
  int         GetCFOEnabled       (HNDLE h_DB, HNDLE h_CFO);
  int         GetCFONEwmPerSecond (HNDLE h_DB, HNDLE h_CFO);

                                        // hostname - short, w/o the domain name
  int         GetDaqHostHandle    (HNDLE Hdb, HNDLE h_CFO, const std::string& Hostname);

                                        // "external" or "emulated"
  int         GetCFOType          (HNDLE Hdb, HNDLE h_CFO);
};

#endif
