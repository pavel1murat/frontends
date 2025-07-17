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
  mu2edaq::DtcInterface* _dtc_i;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqCrvDtc(HNDLE H_RunConf, HNDLE HDtc);
  ~TEqCrvDtc();
  
  virtual TMFeResult Init               () override;
  virtual int        InitVarNames       () override;
  virtual int        ReadMetrics        () override;
};
#endif
