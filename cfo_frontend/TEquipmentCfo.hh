//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TEquipmentCfo_hh__
#define __TEquipmentCfo_hh__

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tmfe.h"
#include "midas.h"

#include "utils/OdbInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"

class TEquipmentCfo : public TMFeEquipment {
  
public:

  OdbInterface*                  _odb_i;
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_frontend_conf;
  HNDLE                          _h_dtc;
  HNDLE                          _h_cfo;
  trkdaq::CfoInterface*          _cfo_i;           // 
  mu2edaq::DtcInterface*         _dtc_i;           //
  int                            _enabled;
  int                            _event_mode;
  int                            _emulated_mode;   // 0:external CFO, 1: emulated CFO
  std::string                    _full_host_name;  // on private network, for communication
  std::string                    _host_label;      // on public network, for ODB
  xmlrpc_env                     _env;

  std::string                    _run_plan_fn;
  int                            _link_mask;        // mask of timing chains (a byte)
  uint64_t                       _n_ewm_train;
  uint64_t                       _first_ts;
  int                            _ew_length;
  int                            _sleep_time_ms;
  int                            _pcie_addr;

  TEquipmentCfo(const char* eqname, const char* eqfilename);

  int InitExternalCfo();
  int InitEmulatedCfo();

  virtual TMFeResult HandleInit         (const std::vector<std::string>& args);
  virtual void       HandlePeriodic     ();
  virtual TMFeResult HandleRpc          (const char* cmd, const char* args, std::string& response);
  virtual TMFeResult HandleBinaryRpc    (const char* cmd, const char* args, std::vector<char>& response);
  virtual TMFeResult HandleBeginRun     (int RunNumber);
  virtual TMFeResult HandleEndRun       (int RunNumber);
  virtual TMFeResult HandlePauseRun     (int RunNumber);
  virtual TMFeResult HandleResumeRun    (int RunNumber);
  virtual TMFeResult HandleStartAbortRun(int RunNumber);

  

};
#endif
