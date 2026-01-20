//-----------------------------------------------------------------------------
// Tracker DTC 
//-----------------------------------------------------------------------------
#ifndef __TEqDisk_hh__
#define __TEqDisk_hh__

#include <ctime>
#include "midas.h"

#include "node_frontend/TMu2eEqBase.hh"

class TEqDisk: public TMu2eEqBase {
public:
  int                   _cmd_run;
  std::time_t           _prev_ctime_sec;
  float                 _prev_fsize_gb;
//-----------------------------------------------------------------------------
// functions
//------------.-----------------------------------------------------------------
  TEqDisk(const char* Name);
  ~TEqDisk();

  virtual TMFeResult  Init          () override;
  virtual int         InitVarNames  () override;
  virtual int         HandlePeriodic() override;

  int                 ReadMetrics   ();

  static void         ProcessCommand(int hDB, int hKey, void* Info);
  
};
#endif
