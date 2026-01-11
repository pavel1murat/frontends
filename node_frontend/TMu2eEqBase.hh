//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TMu2eEqBase_hh__
#define __TMu2eEqBase_hh__

#include <ctime>
#include "midas.h"
#include "tmfe.h"

class OdbInterface;

class TMu2eEqBase {
public:
  std::string   _name;                  // equipment name, for now: 'dtc0','dtc1','artdaq','disk'
  std::string   _full_host_name;
  std::string   _host_label;            // i.e. 'mu2edaq09'
  std::string   _logfile;               // set by the derived class
  HNDLE         _h_active_run_conf;
  HNDLE         _h_daq_host_conf;

  OdbInterface* _odb_i;
                                        // there could be have more monitoring flags - in the
                                        // derives classes, below are the common ones
  int           _monitoringLevel;       // 0 or 1
  int           _diagLevel;             // always needed
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TMu2eEqBase(const char* Name);
  virtual ~TMu2eEqBase();

  std::string&        Name        () { return _name          ; }
  std::string&        HostLabel   () { return _host_label    ; }
  std::string&        FullHostName() { return _full_host_name; }

  OdbInterface*       Odb_i       () { return _odb_i         ; }

  int                 MonitoringLevel() { return _monitoringLevel ; }

  int                 ResetOutput ();
  int                 WriteOutput (const std::string& Output, const std::string& Logfile = "");

  void SetName(const char* Name) { _name = Name; }

  virtual TMFeResult  Init               ();
  virtual int         InitVarNames       ();
  virtual int         ReadMetrics        ();
  virtual int         BeginRun           (HNDLE H_RunConf);
};
#endif
