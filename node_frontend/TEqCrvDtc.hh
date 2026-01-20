//-----------------------------------------------------------------------------
// CRV DTC 
//-----------------------------------------------------------------------------
#ifndef __TEqCrvDtc_hh__
#define __TEqCrvDtc_hh__

#include <ctime>
#include "midas.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "node_frontend/TMu2eEqBase.hh"

class TEqCrvDtc: public TMu2eEqBase {
public:
  HNDLE                   _h_dtc;
  mu2edaq::DtcInterface*  _dtc_i;
  int                     _cmd_run;
  int                     _monitorRocRegisters;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqCrvDtc(const char* Name, HNDLE H_RunConf, HNDLE HDtc);
  ~TEqCrvDtc();
  
  mu2edaq::DtcInterface* Dtc_i() { return _dtc_i; }

  virtual TMFeResult Init               () override;
  virtual int        InitVarNames       () override;
  virtual int        HandlePeriodic     () override;
  
  // int                ReadMetrics        ();

  virtual int        BeginRun         (HNDLE H_RunConf) override;

  int                ConfigureJA      (std::ostream& Stream);
  int                InitReadout      (std::ostream& Stream);
  int                PrintRocStatus   (std::ostream& Stream);
  int                ReadRegister     (std::ostream& Stream);
  int                ReadRocRegister  (std::ostream& Stream);
  int                ResetRoc         (std::ostream& Stream);
  int                WriteRocRegister (std::ostream& Stream);
  int                WriteRegister    (std::ostream& Stream);

  static void        ProcessCommand   (int hDB, int hKey, void* Info);
};
#endif
