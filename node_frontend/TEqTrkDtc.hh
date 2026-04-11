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
                                        // read ROC registers listed in .cc file - for monitoring ? and
                                        // saving in the end of the run purposes
  
  int         ReadRocRegisters (int Link, const std::vector<int>& Registers, std::vector<uint32_t>& RegData);
  void        SetLinkStatus(int Link, int Status);
  
  virtual int BeginRun         (int RunNumber) override;
  virtual int EndRun           (int RunNumber) override;
  
  int         ConfigureJA      (HNDLE H_Cmd);
  int         DigiRead         (HNDLE H_Cmd);
  int         DigiRW           (HNDLE H_Cmd);
  int         DigiWrite        (HNDLE H_Cmd);
  int         DumpSettings     (HNDLE H_Cmd);
  int         FindAlignment    (HNDLE H_Cmd);
  int         FindThresholds   (HNDLE H_Cmd);
  int         GetKey           (HNDLE H_Cmd);
  int         GetRocDesignInfo (HNDLE H_Cmd);
  int         HardReset        (HNDLE H_Cmd);
  int         InitReadout      (HNDLE H_Cmd);
  int         LoadChannelMap   (HNDLE H_Cmd);                 // load map from disk to ODB
  int         LoadThresholds   (HNDLE H_Cmd);                 // load thresholds from disk to ODB
  int         MeasureThresholds(HNDLE H_Cmd);                 // std::ostream& Stream);
  int         PrintRocStatus   (HNDLE H_Cmd);
  int         PrintStatus      (HNDLE H_Cmd);
  int         PulserOff        (HNDLE H_Cmd);
  int         PulserOn         (HNDLE H_Cmd);
  int         Rates            (HNDLE H_Cmd);
  int         Read             (HNDLE H_Cmd);
  int         ReadDDR          (HNDLE H_Cmd);
  int         ReadDeviceID     (HNDLE H_Cmd);
  int         ReadIlp          (HNDLE H_Cmd);
  int         ReadMnID         (HNDLE H_Cmd);
  int         ReadRegister     (HNDLE H_Cmd);
  int         ReadRocRegister  (HNDLE H_Cmd);
  int         ReadSpi          (HNDLE H_Cmd);
  int         ReadSubevents    (HNDLE H_Cmd);
  int         RebootMcu        (HNDLE H_Cmd);
  int         ResetDigis       (HNDLE H_Cmd);
  int         ResetRoc         (HNDLE H_Cmd);
  int         SaveChannelMap   (HNDLE H_Cmd);                 // save active channel map from ODB to disk
  int         SaveThresholds   (HNDLE H_Cmd);                 // save thresholds from disk to ODB
  int         SetCalDac        (HNDLE H_Cmd);
  int         SetRocDelay      (HNDLE H_Cmd);
  int         SetThresholds    (HNDLE H_Cmd);                 // load thresholds from ODB to firmware
  int         SoftReset        (HNDLE H_Cmd);
  int         TestCommand      (HNDLE H_Cmd);
  int         WriteRegister    (HNDLE H_Cmd);
  int         WriteRocRegister (HNDLE H_Cmd);

  static void ProcessCommand   (int hDB, int hKey, void* Info);
  
  virtual int StartMessage     (HNDLE h_Cmd, std::stringstream& Stream) override;
};
#endif
