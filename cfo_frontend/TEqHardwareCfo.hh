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
  
  struct time_chain_node_t {
    std::string   node_dtc;
    int           finished{0};
  };
  

  trkdaq::CfoInterface*                 _cfo_i;        // 
  int                                   _pcie_addr;    // 
  int                                   _cmd_run;      // storage used by the ODB callback

  std::vector<std::vector<time_chain_node_t>> _timing_chain;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqHardwareCfo(const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_CfoConf);
  ~TEqHardwareCfo();

  trkdaq::CfoInterface* Cfo_i() { return _cfo_i; }

  int PcieAddr() { return _pcie_addr; }

  virtual TMFeResult    Init ()               override;
  
  int          ConfigureJA    (HNDLE H_Cmd);
  int          CompileRunPlan (HNDLE H_Cmd);
  int          Halt           (HNDLE H_Cmd);
  int          HardReset      (HNDLE H_Cmd);
  int          InitReadout    (HNDLE H_Cmd);
  int          InitTimingChain(HNDLE H_Cmd);
  int          LaunchRunPlan  (HNDLE H_Cmd);
  int          PrintStatus    (HNDLE H_Cmd);
  int          ReadRegister   (HNDLE H_Cmd);
  int          SetRunPlan     (HNDLE H_Cmd);
  int          SoftReset      (HNDLE H_Cmd);
  int          WriteRegister  (HNDLE H_Cmd);

  int          GetTimingChainTopology();
  
  static  void ProcessCommand(int hDB, int hKey, void* Info);

  int          WaitForCompletion(HNDLE h_Cmd);

  virtual int  HandlePeriodic()               override;
  virtual int  BeginRun       (int RunNumber) override;
  virtual int  EndRun         (int RunNumber) override;

  virtual int  StartMessage  (HNDLE h_Cmd, std::stringstream& Stream) override;
};
#endif
