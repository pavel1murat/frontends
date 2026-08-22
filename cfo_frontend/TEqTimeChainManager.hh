//-----------------------------------------------------------------------------
// hardware CFO
// 1) at begin run, launches the run plan,
// 2) at end run, stops the run plan that's it
//-----------------------------------------------------------------------------
#ifndef __TEqTimeChainManager_hh__
#define __TEqTimeChainManager_hh__

// #include "xmlrpc-c/config.h"  /* information about this build environment */
// #include <xmlrpc-c/base.h>
// #include <xmlrpc-c/client.h>

#include "tmfe.h"
#include "midas.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"

#include "utils/OdbInterface.hh"
#include "utils/TMu2eEqBase.hh"

class TEqTimeChainManager : public TMu2eEqBase {
  
public:

  int                            _cmd_run;      // storage used by the ODB callback
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqTimeChainManager(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_CfoConf);
  ~TEqTimeChainManager();

  virtual TMFeResult    Init           ()    override;
  
  virtual int  HandlePeriodic()              override;
  virtual int  BeginRun      (int RunNumber) override;
  virtual int  EndRun        (int RunNumber) override;

  int          InitReadout      (HNDLE H_Cmd);

  static  void ProcessCommand(int hDB, int hKey, void* Info);
  virtual int  StartMessage  (HNDLE h_Cmd, std::stringstream& Stream) override;

};
#endif
