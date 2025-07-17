//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TMu2eEqBase_hh__
#define __TMu2eEqBase_hh__

#include <ctime>
#include "midas.h"
#include "tmfe.h"

class TMu2eEqBase {
public:
  std::string   _name;
  std::string   _full_host_name;
  std::string   _host_label;
  std::string   _logfile;               // set by the derived class
  HNDLE         _h_daq_cost_conf;
                                        // there could be have more monitoring flags - in the
                                        // derives classes, below are the common ones
  int           _monitorDtc;
  int           _monitorDisk;
  int           _monitorArtdaq;
                                        // alway s need a diag flag
  int           _diagLevel;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TMu2eEqBase();
  virtual ~TMu2eEqBase();

  std::string&        Name        () { return _name          ; }
  std::string&        HostLabel   () { return _host_label    ; }
  std::string&        FullHostName() { return _full_host_name; }

  void                ResetOutput ();
  void                WriteOutput (const std::string& Output);

  virtual TMFeResult  Init               ();
  virtual int         InitVarNames       ();
  virtual int         ReadMetrics        ();
  virtual int         BeginRun           (HNDLE H_RunConf);
};
#endif
