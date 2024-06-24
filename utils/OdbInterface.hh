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

                                        // returns: 1="external" or 0="emulated"
  int         GetCFOExternal      (HNDLE Hdb, HNDLE h_CFO);

                                        // N events to be generated in emulated mode
  int         GetNEvents          (HNDLE Hdb, HNDLE h_CFO);

                                        // event window size in units of 25 ns (40 MHz)
  int         GetEventWindowSize  (HNDLE Hdb, HNDLE h_CFO);
};

#endif
