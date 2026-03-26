//-----------------------------------------------------------------------------
// hardware CFO
// 1) at begin run, launches the run plan,
// 2) at end run, stops the run plan that's it
//-----------------------------------------------------------------------------
#ifndef __TEqHardwareCfo_hh__
#define __TEqHardwareCfo_hh__

// #include "xmlrpc-c/config.h"  /* information about this build environment */
// #include <xmlrpc-c/base.h>
// #include <xmlrpc-c/client.h>

#include "tmfe.h"
#include "midas.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"

#include "utils/OdbInterface.hh"
#include "utils/TMu2eEqBase.hh"

class TEqHardwareCfo : public TMu2eEqBase {
  
public:

  trkdaq::CfoInterface*          _cfo_i;        // 
  int                            _pcie_addr;    // 
  int                            _cmd_run;      // storage used by the ODB callback
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqHardwareCfo(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_CfoConf);
  ~TEqHardwareCfo();

  trkdaq::CfoInterface* Cfo_i() { return _cfo_i; }

  int PcieAddr() { return _pcie_addr; }

  virtual TMFeResult    Init           ()    override;
  
  virtual int  HandlePeriodic()              override;
  virtual int  BeginRun      (int RunNumber) override;
  virtual int  EndRun        (int RunNumber) override;

  int          ConfigureJA   (HNDLE H_Cmd);
  int          CompileRunPlan(HNDLE H_Cmd);
  int          Halt          (HNDLE H_Cmd);
  int          InitReadout   (HNDLE H_Cmd);
  int          LaunchRunPlan (HNDLE H_Cmd);
  int          ReadRegister  (HNDLE H_Cmd);
  int          WriteRegister (HNDLE H_Cmd);

  static  void ProcessCommand(int hDB, int hKey, void* Info);
  virtual int  StartMessage  (HNDLE h_Cmd, std::stringstream& Stream) override;

};
#endif
