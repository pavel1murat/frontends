//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __utils_TMu2eEqBase_hh__
#define __utils_TMu2eEqBase_hh__

#include <ctime>
#include "midas.h"
#include "tmfe.h"

class OdbInterface;

class TMu2eEqBase {
public:
                                        // subsystem? generic equipment type ? - will see.. 0=undefined
  enum {
    kUndefined   = 0,
    kTracker     = 1,
    kCalorimeter = 2,
    kCrv         = 3,
    kStm         = 4,
    kExm         = 5,
    kArtdaq      = 6,
    kDisk        = 7,
  };
  
  std::string   _name;                  // equipment name, for now: 'DTC0','DTC1','ARTDAQ','DISK' (capitalized)
  std::string   _title;                 // name-related stub in ODB : 'DTC0', 'DTC1', 'Artdaq', 'Disk', etc
  std::string   _full_host_name;
  std::string   _host_label;            // i.e. 'mu2edaq09'
  std::string   _logfile;               // set by the derived class
  HNDLE         _h_active_run_conf;
  HNDLE         _h_daq_host_conf;

  OdbInterface* _odb_i;

  int           _subsystem;             // subsystem
                                        // there could be have more monitoring flags - in the
                                        // derives classes, below are the common ones
  int           _monitoringLevel;       // 0 or 1
  int           _diagLevel;             // always needed
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TMu2eEqBase(const char* Name, const char* Title, int Subsystem = kUndefined);
  virtual ~TMu2eEqBase();

  std::string&        Name        () { return _name          ; }
  std::string&        Title       () { return _title         ; }
  int                 Subsystem   () { return _subsystem     ; }
  std::string&        HostLabel   () { return _host_label    ; }
  std::string&        FullHostName() { return _full_host_name; }

  OdbInterface*       Odb_i       () { return _odb_i         ; }

  int                 MonitoringLevel() { return _monitoringLevel ; }

  int                 ResetOutput (const std::string& Logfile = "");
  int                 WriteOutput (const std::string& Output, const std::string& Logfile = "");

  void SetName(const char* Name) { _name = Name; }

  virtual TMFeResult  Init               ();
  virtual int         InitVarNames       ();
  virtual int         HandlePeriodic     ();
  //  virtual int         ReadMetrics        ();
  virtual int         BeginRun           (HNDLE H_RunConf);
};
#endif
