//-----------------------------------------------------------------------------
// Tracker DTC 
//-----------------------------------------------------------------------------
#ifndef __TEqTrkDtc_hh__
#define __TEqTrkDtc_hh__

#include <ctime>
#include "midas.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "utils/TMu2eEqBase.hh"

class TEqTrkDtc: public TMu2eEqBase {
  enum {
    kNRegHist    =  4,
    kNRegNonHist = 15,     
  } ;
 
public:
  HNDLE                 _h_dtc;
  trkdaq::DtcInterface* _dtc_i;
  int                   _cmd_run;
  int                   _monitorRocRegisters;
  int                   _monitorRates;
  int                   _monitorSPI;
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
  TEqTrkDtc (const char* Name, const char* Title);
  TEqTrkDtc (const char* Name, const char* Title, HNDLE H_RunConf, HNDLE H_Dtc);
  ~TEqTrkDtc();

  trkdaq::DtcInterface* Dtc_i() { return _dtc_i; }

  void ReadNonHistRegisters();
  
  TMFeResult  Init             () override;
  virtual int InitVarNames     () override;
  virtual int HandlePeriodic   () override;

  int         ReadMetrics      ();
  
  virtual int BeginRun         (HNDLE H_RunConf) override;
  
  int         ConfigureJA      (std::ostream& Stream);
  int         DigiRW           (std::ostream& Stream);
  int         DumpSettings     (std::ostream& Stream);
  int         FindAlignment    (HNDLE H_Cmd);
  int         FindThresholds   (std::ostream& Stream);
  int         GetKey           (std::ostream& Stream);
  int         GetRocDesignInfo (std::ostream& Stream);
  int         InitReadout      (std::ostream& Stream);
  int         LoadThresholds   (HNDLE H_Cmd);                 // load thresholds from disk to ODB
  int         MeasureThresholds(std::ostream& Stream);
  int         PrintRocStatus   (std::ostream& Stream);
  int         ProgramRoc       (std::ostream& Stream);
  int         PulserOff        (std::ostream& Stream);
  int         PulserOn         (std::ostream& Stream);
  int         Rates            (std::ostream& Stream);
  int         Read             (std::ostream& Stream);
  int         ReadDDR          (std::ostream& Stream);
  int         ReadIlp          (std::ostream& Stream);
  int         ReadMnID         (std::ostream& Stream);
  int         ReadRegister     (std::ostream& Stream);
  int         ReadRocRegister  (std::ostream& Stream);
  int         ReadSpi          (std::ostream& Stream);
  int         ResetRoc         (std::ostream& Stream);
  int         RebootMcu        (std::ostream& Stream);
  int         SetCalDac        (std::ostream& Stream);
  int         SetThresholds    (HNDLE H_Cmd);  // ODB --> preamps
  int         StartMessage     (HNDLE h_Cmd, std::ostream& Stream);
  int         TestCommand      (std::ostream& Stream);
  int         WriteRocRegister (std::ostream& Stream);
  int         WriteRegister    (std::ostream& Stream);

  static void ProcessCommand   (int hDB, int hKey, void* Info);
  
};
#endif
