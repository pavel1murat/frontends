//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TEquipmentNode_hh__
#define __TEquipmentNode_hh__

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tmfe.h"
#include "midas.h"

#include "utils/OdbInterface.hh"
#include "node_frontend/ArtdaqComponent.hh"
#include "node_frontend/ArtdaqMetrics.hh"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

class TEquipmentNode : public TMFeEquipment {
  enum {
    kNRegHist    =  4,
    kNRegNonHist = 15,     
  } ;
//-----------------------------------------------------------------------------
//                                            Temp, VCCINT, VCCAUX, VCBRAM
  // SC TODO: I think we should define these together with the registers? 
  std::initializer_list<const char*> DtcRegHistNames = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};
  std::initializer_list<int> DtcRegHist =              { 0x9010, 0x9014,    0x9018,   0x901c};
  
  std::initializer_list<int>  DtcRegisters = {
    0x9004, 0x9100, 0x9114, 0x9140, 0x9144,
    0x9158, 0x9188, 0x91a8, 0x91ac, 0x91bc, 
    0x91c0, 0x91c4, 0x91f4, 0x91f8, 0x93e0
  };
  
public:

  HNDLE                          hDB;
  HNDLE                          _h_active_run_conf;
  HNDLE                          _h_daq_host_conf;
  HNDLE                          _h_frontend_conf;
  mu2edaq::DtcInterface*          fDtc_i[2];       // one or two DTCs, nullprt:disabled
  std::vector<ArtdaqComponent_t> _list_of_ac;
  std::string                    _full_host_name;  // on private network, for communicatioin
  std::string                    _host_label;      // on public network, for ODB
  xmlrpc_env                     _env;
  int                            _monitorDtc;
  int                            _monitorArtdaq;
  int                            _monitorRocSPI;        // trk specific, these are history channels
  int                            _monitorRoc;           // hisotry channels
  int                            _monitorRocRegisters;  // non-hisotry channels
  OdbInterface*                  _odb_i;

  TEquipmentNode(const char* eqname, const char* eqfilename);

  virtual TMFeResult HandleInit         (const std::vector<std::string>& args);
  virtual void       HandlePeriodic     ();
  virtual TMFeResult HandleRpc          (const char* cmd, const char* args, std::string& response);
  virtual TMFeResult HandleBinaryRpc    (const char* cmd, const char* args, std::vector<char>& response);
  virtual TMFeResult HandleBeginRun     (int RunNumber);
  virtual TMFeResult HandleEndRun       (int RunNumber);
  virtual TMFeResult HandlePauseRun     (int RunNumber);
  virtual TMFeResult HandleResumeRun    (int RunNumber);
  virtual TMFeResult HandleStartAbortRun(int RunNumber);
  
  TMFeResult         InitDtc            ();
  void               InitDtcVarNames    ();
  void               ReadDtcMetrics     ();

  TMFeResult         InitArtdaq             ();
  void               InitArtdaqVarNames     ();
  void               ReadArtdaqMetrics      ();
  int                ReadBrMetrics          (const ArtdaqComponent_t* Ac);
  int                ReadDataReceiverMetrics(const ArtdaqComponent_t* Ac);
  int                ReadDsMetrics          (const ArtdaqComponent_t* Ac);

};
#endif
