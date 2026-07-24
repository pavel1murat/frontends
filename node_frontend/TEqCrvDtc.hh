//-----------------------------------------------------------------------------
// Tracker DTC 
//-----------------------------------------------------------------------------
#ifndef __TEqCrvDtc_hh__
#define __TEqCrvDtc_hh__

#include <ctime>
#include "midas.h"

// #include "frontends/crv/DtcInterfaceCrv.hh"
#include "crv/DtcInterfaceCrv.hh"
#include "utils/TMu2eEqBase.hh"

class TEqCrvDtc: public TMu2eEqBase {
  enum {
    kNRegHist    =  4,
    kNRegNonHist = 15,     
  } ;
 
public:
  HNDLE                  _h_dtc;
  mu2edaq::DtcInterface* _dtc_i;
  int                    _monitorRocRegisters;
  int                    _monitorRates;
  int                    _monitorSPI;
//-----------------------------------------------------------------------------
// threads - perhaps, not needed any more
//-----------------------------------------------------------------------------
  struct ThreadContext_t {
    int                             fPcieAddr;
    int                             fLink;
    int                             fRunning;          // status: 0=stopped 1=running
    int                             fStop;             // end marker
    int                             fCmd;              // command
    int                             fPrintLevel;

    ThreadContext_t() {}
    
    ThreadContext_t(int PcieAddr, int Link, int PrintLevel = 0): 
      fPcieAddr (PcieAddr), fLink(Link), fPrintLevel(PrintLevel) {}
  };
  
  ThreadContext_t                   fSetThrContext;
  std::stringstream                 fSSthr;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqCrvDtc (const char* Name, const char* Title);
  TEqCrvDtc (const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Dtc);
  ~TEqCrvDtc();

  mu2edaq::DtcInterface* Dtc_i() { return _dtc_i; }

  void ReadNonHistRegisters();
  
  TMFeResult  Init             () override;
  virtual int InitVarNames     () override;
  virtual int HandlePeriodic   () override;
  virtual int MonitoringLevel  () override;

  int         ReadMetrics      ();
                                        // read ROC registers listed in .cc file - for monitoring ? and
                                        // saving in the end of the run purposes
  
  int         ReadRocRegisters (int Link, const std::vector<int>& Registers, std::vector<uint32_t>& RegData);
  void        SetLinkStatus(int Link, int Status);
  
  virtual int BeginRun         (int RunNumber) override;
  virtual int EndRun           (int RunNumber) override;
  
  int         ConfigureJA      (HNDLE H_Cmd);
  int         ClearStatus      (HNDLE H_Cmd);
  int         HardReset        (HNDLE H_Cmd);
  int         InitReadout      (HNDLE H_Cmd);
  int         PrintRocStatus   (HNDLE H_Cmd);
  int         PrintStatus      (HNDLE H_Cmd);
  int         ReadRegister     (HNDLE H_Cmd);
  int         ReadRocRegister  (HNDLE H_Cmd);
  int         ReadSubevents    (HNDLE H_Cmd);
  int         ResetRoc         (HNDLE H_Cmd);
  int         SoftReset        (HNDLE H_Cmd);
  int         WriteRegister    (HNDLE H_Cmd);
  int         WriteRocRegister (HNDLE H_Cmd);

  static void ProcessCommand   (int hDB, int hKey, void* Info);
  
  virtual int StartMessage     (HNDLE h_Cmd, std::stringstream& Stream) override;
};
#endif
