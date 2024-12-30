///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
#include "artdaq/DAQdata/Globals.hh"
#define TRACE_NAME "TrackerBRDR"

#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "artdaq/Generators/GeneratorMacros.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core-mu2e/Overlays/FragmentType.hh"
#include "artdaq-core-mu2e/Overlays/TrkDtcFragment.hh"

#include <fstream>
#include <iomanip>
#include <iterator>

#include <unistd.h>

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/Generators/CommandableFragmentGenerator.hh"
#include "fhiclcpp/fwd.h"

#include "dtcInterfaceLib/DTC.h"
// #include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#ifdef USE_MIDAS
#include "midas.h"
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#endif

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
using namespace DTCLib;
//-----------------------------------------------------------------------------
// ARTDAQ messaging - XML-RPC
//-----------------------------------------------------------------------------
#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#define DEFAULT_FE_TIMEOUT  10000       /* 10 seconds for watchdog timeout */

namespace mu2e {
  class TrackerBRDR : public artdaq::CommandableFragmentGenerator {

    enum {
      kReadDigis   = 0,
      kReadPattern = 1
    };
//-----------------------------------------------------------------------------
// FHiCL-configurable variables. 
// C++ variable names are the FHiCL parameter names prepended with a "_"
//-----------------------------------------------------------------------------
    std::string                           _artdaqLabel;
    std::chrono::steady_clock::time_point _lastReportTime;
    std::chrono::steady_clock::time_point _procStartTime;
    std::vector<uint16_t>                 _fragment_ids;       // handled by CommandableGenerator,
                                                               //  but not a data member there
    std::string                           _sFragmentType;
    int                                   _debugLevel;
    size_t                                _nEventsDbg;
    std::string                           _tfmHost;            // used to send xmlrpc messages to
    int                                   _pcieAddr;

                                                               // 101:simulate data internally, DTC not used; default:0

    int                                   _readData;           // 1: read data, 0: save empty fragment
    int                                   _readDTCRegisters;   // 1: read and save the DTC registers
    int                                   _printFreq;          // printout frequency
    int                                   _maxEventsPerSubrun; // 
    int                                   _readoutMode;        // 0:digis; 1:ROC pattern (all defined externally); 

    trkdaq::DtcInterface*                 _dtc_i;
    DTCLib::DTC*                          _dtc;
    FragmentType                          _fragmentType;

    uint16_t                              _reg[200];           // DTC registers to be saved
    int                                   _nreg;               // their number
    xmlrpc_env                            _env;                // XML-RPC environment
    ulong                                 _tstamp;             // 
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  public:
    explicit TrackerBRDR(fhicl::ParameterSet const& ps);
    virtual ~TrackerBRDR();
    
  private:
    // The "getNext_" function is used to implement user-specific
    // functionality; it's a mandatory override of the pure virtual
    // getNext_ function declared in CommandableFragmentGenerator
    
    bool readEvent    (artdaq::FragmentPtrs& output);
    bool simulateEvent(artdaq::FragmentPtrs& output);  
    bool getNext_     (artdaq::FragmentPtrs& output) override;

    bool sendEmpty_   (artdaq::FragmentPtrs& output);

    void start      () override {}
    void stopNoMutex() override {}
    void stop       () override;
    
    //    void print_dtc_registers(DTC* Dtc, const char* Header);
    void printBuffer        (const void* ptr, int sz);
//-----------------------------------------------------------------------------
// try follow Simon ... perhaps one can improve on bool? 
// also do not pass strings by value
//-----------------------------------------------------------------------------
    int  message(const std::string& msg_type, const std::string& message);
                                        // read functions
    int  readData        (artdaq::FragmentPtrs& Frags, ulong& Timestamp);

    double _timeSinceLastSend() {
      auto now        = std::chrono::steady_clock::now();
      auto deltaw     = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(now - _lastReportTime).count();
      _lastReportTime = now;
      return deltaw;
    }
    
    void   _startProcTimer() { _procStartTime = std::chrono::steady_clock::now(); }

//-----------------------------------------------------------------------------
// - the first one came from the generator template, 
// - the second one - comments in the CommandableFragmentGenerator.hh
// - the base class provides the one w/o the underscore 
// - the comments seem to have a general confusion 
//   do we really need both ?
//-----------------------------------------------------------------------------
    std::vector<uint16_t>         fragmentIDs_() { return _fragment_ids; }
    virtual std::vector<uint16_t> fragmentIDs () override;
    
    double _getProcTimerCount() {
      auto now = std::chrono::steady_clock::now();
      auto deltaw =
        std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(now - _procStartTime).count();
      return deltaw;
    }
  };
}  // namespace mu2e


//-----------------------------------------------------------------------------
// define allowed fragment types( = ID's)
//-----------------------------------------------------------------------------
std::vector<uint16_t> mu2e::TrackerBRDR::fragmentIDs() {
  std::vector<uint16_t> v;
  v.push_back(0);
  if (_readDTCRegisters) v.push_back(FragmentType::TRKDTC);
  
  return v;
}

//-----------------------------------------------------------------------------
// sim_mode="N" means real DTC 
//-----------------------------------------------------------------------------
mu2e::TrackerBRDR::TrackerBRDR(fhicl::ParameterSet const& ps)
  : CommandableFragmentGenerator(ps)
  , _artdaqLabel       (ps.get<std::string>             ("artdaqLabel"                     ))
  , _lastReportTime    (std::chrono::steady_clock::now())
  , _fragment_ids      (ps.get<std::vector<uint16_t>>   ("fragment_ids"       , std::vector<uint16_t>()))  // 
  , _sFragmentType     (ps.get<std::string>             ("fragmentType"       ,       "TRK"))  // 
  , _debugLevel        (ps.get<int>                     ("debugLevel"         ,           0))
  , _nEventsDbg        (ps.get<size_t>                  ("nEventsDbg"         ,         100))
  , _tfmHost           (ps.get<std::string>             ("tfmHost"                         ))  // 
  , _readData          (ps.get<int>                     ("readData"           ,           1))  // 
  , _printFreq         (ps.get<int>                     ("printFreq"          ,         100))  // 
  , _maxEventsPerSubrun(ps.get<int>                     ("maxEventsPerSubrun" ,       10000))  // 
  , _readoutMode       (ps.get<int>                     ("readoutMode"        ,           1))  // 
  
{
  _fragmentType = mu2e::toFragmentType(_sFragmentType);
  TLOG(TLVL_INFO) << "label:" << _artdaqLabel << " CONSTRUCTOR (1) readData:" << _readData
                  << " fragmentType:" << _sFragmentType << ":" << int(_fragmentType);
//-----------------------------------------------------------------------------
// the BR interface should not be changing any settings, just read events
// DTC is already initialized by the frontend, don't change anything !
// for skip_init=true, the linkmask is not used. The board reader shouldn't even know
// about it, but it has to know about the DTC PCIE address
//-----------------------------------------------------------------------------
#ifdef USE_MIDAS
//-----------------------------------------------------------------------------
// figure out the PCIE address - nothing wrong with connecting to ODB and
//-----------------------------------------------------------------------------
  char host_name[100], exp_name[100];
  cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));
  if (host_name[0] == 0)  {
    std::string host = get_short_host_name("local");
    sprintf(host_name, host.data());
  }

  TLOG(TLVL_INFO) << "label: " << _artdaqLabel << " host name: " << host_name << " , exp_name: " << exp_name;

  int status = cm_connect_experiment(_tfmHost.data(), exp_name, _artdaqLabel.data(),NULL);
  if (status != CM_SUCCESS) {
    cm_msg(MERROR, _artdaqLabel.data(),
           "Cannot connect to experiment \'%s\' on host \'%s\', status %d",
           exp_name,host_name,status);
    TLOG(TLVL_ERROR) << "label: " << _artdaqLabel << " ERROR: failed to connect to experiment. BAIL OUT";

    /* let user read message before window might close */
    ss_sleep(5000);
    return;
  }
  
  HNDLE  hDB(0);
  HNDLE  hClient(0);
  cm_get_experiment_database(&hDB, &hClient);

  TLOG(TLVL_INFO) << "label: " << _artdaqLabel << " hDB:" << hDB << " hClient:" <<  hClient;

  if (hDB == 0) {
    TLOG(TLVL_ERROR) << "label: " << _artdaqLabel << " ERROR: failed to connect to ODB. BAIL OUT";
    return;
  }

  OdbInterface* odb_i         = OdbInterface::Instance(hDB);
  
  HNDLE h_active_run_conf     = odb_i->GetActiveRunConfigHandle();
  std::string active_run_conf = odb_i->GetRunConfigName(h_active_run_conf);
  std::string rpc_host        = get_short_host_name("local");
  HNDLE h_host_artdaq_conf    = odb_i->GetDaqHostHandle(hDB,h_active_run_conf,rpc_host+"/Artdaq");

  TLOG(TLVL_INFO) << "label: " << _artdaqLabel
                  << " h_active_run_conf:" << h_active_run_conf
                  << " active_run_conf_name:" << active_run_conf
                  << " rpc_host:" << rpc_host << " h_host_artdaq_conf:" << h_host_artdaq_conf;
//-----------------------------------------------------------------------------
// a DAQ host has keys: "DTC0", DTC1", and "Artdaq"
//-----------------------------------------------------------------------------
  HNDLE h_component;
  KEY   component;
  int   pcie_addr(-1);

  for (int i=0; db_enum_key(hDB, h_host_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
    db_get_key(hDB, h_component, &component);
    TLOG(TLVL_INFO) << "label:"  << _artdaqLabel
                    << " Subkey:" <<  component.name
                    << " Type:"  << component.type;
    if (component.name == _artdaqLabel) {
//-----------------------------------------------------------------------------
// the board reader configuration found, take the PCIE address from there
//-----------------------------------------------------------------------------
      pcie_addr = odb_i->GetDtcPcieAddress(hDB,h_component);
      TLOG(TLVL_INFO) << "label: " << _artdaqLabel << " pcie_addr from ODB: " << pcie_addr;
      break;
    }
  }
  if (pcie_addr != -1) _pcieAddr = pcie_addr;

  _tfmHost = odb_i->GetTfmHostName(hDB,h_active_run_conf);
  TLOG(TLVL_INFO) << "label:"               << _artdaqLabel
                  << " active_run_conf:"    << active_run_conf
                  << " _tfmHost_from_ODB:"  << _tfmHost;
  
  cm_disconnect_experiment();
#endif
  
  bool skip_init(true);
  _dtc_i = trkdaq::DtcInterface::Instance(_pcieAddr,0x111111,skip_init);
  _dtc   = _dtc_i->Dtc();
//-----------------------------------------------------------------------------
// finally, initialize the environment for the XML-RPC messaging client
//-----------------------------------------------------------------------------
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "debug", "v1_0");
  xmlrpc_env_init(&_env);

  _tstamp           = 0;
  _readDTCRegisters = 0;
}

//-----------------------------------------------------------------------------
// let the boardreader send messages back to the TFM and report problems 
// so far, make the TFM hist a talk-to parameter
// GetPartitionNumber() is an artdaq global function - see artdaq/artdaq/DAQdata/Globals.hh
//-----------------------------------------------------------------------------
int mu2e::TrackerBRDR::message(const std::string& msg_type, const std::string& message) {
    
  auto _xmlrpcUrl = "http://" + _tfmHost + ":" + std::to_string((10000 +1000 * GetPartitionNumber()))+"/RPC2";

  xmlrpc_client_call(&_env, _xmlrpcUrl.data(), "message","(ss)", msg_type.data(), 
                     (artdaq::Globals::app_name_+":"+message).data());
  if (_env.fault_occurred) {
    TLOG(TLVL_ERROR) << "label: " << _artdaqLabel << " XML-RPC rc=" << _env.fault_code << " " << _env.fault_string;
    return -1;
  }
  else {
    TLOG(TLVL_DEBUG) << "label: " << _artdaqLabel << "message successfully sent. type:" << msg_type << " message" << message;
  }
  return 0;
}

//-----------------------------------------------------------------------------
mu2e::TrackerBRDR::~TrackerBRDR() {
} 

//-----------------------------------------------------------------------------
void mu2e::TrackerBRDR::stop() {
}

//-----------------------------------------------------------------------------
// read one event - could consist of multiple DTC blocks (subevents)
//-----------------------------------------------------------------------------
int mu2e::TrackerBRDR::readData(artdaq::FragmentPtrs& Frags, ulong& TStamp) {

  int    rc         (-1);                        // return code, negative if error
  bool   timeout    (false);
  bool   readSuccess(false);
  bool   match_ts   (false);
  int    nbytes     (0);
  std::vector<std::unique_ptr<DTCLib::DTC_SubEvent>> subevents;  // auto   tmo_ms(1500);

  TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                   << " START TStamp:" << TStamp << std::endl;

  DTC_EventWindowTag event_tag = DTC_EventWindowTag(_tstamp);

  try {
//------------------------------------------------------------------------------
// sz: 1 (or 0, if nothing has been read out)
//-----------------------------------------------------------------------------
      subevents = _dtc->GetSubEventData(event_tag,match_ts);
      int sz    = subevents.size();
      TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                       << " read:" << sz
                       << " DTC blocks" << std::endl;
      if (sz > 0) {
        _tstamp  += 1;
      }
//-----------------------------------------------------------------------------
// each subevent (a block of data corresponding to a single DTC) becomes an artdaq fragment
//-----------------------------------------------------------------------------
      for (int i=0; i<sz; i++) {
        DTC_SubEvent* ev     = subevents[i].get();
        int           nb     = ev->GetSubEventByteCount();
        uint64_t      ew_tag = ev->GetEventWindowTag().GetEventWindowTag(true);

        TStamp = ew_tag;  // hack
        
        TLOG(TLVL_DEBUG) << "label:"      << _artdaqLabel
                         << " dtc_block:" << i
                         << " nbytes:" << nb << std::endl;
        nbytes += nb;
        if (nb > 0) {
          artdaq::Fragment* frag = new artdaq::Fragment(ev_counter(), _fragment_ids[0], _fragmentType, TStamp);

          frag->resizeBytes(nb);
      
          void* afd  = frag->dataBegin();

          memcpy(afd,ev->GetRawBufferPointer(),nb);
          Frags.emplace_back(frag);
//-----------------------------------------------------------------------------
// this is essentially it, now - diagnostics 
//-----------------------------------------------------------------------------
          uint64_t ew_tag = ev->GetEventWindowTag().GetEventWindowTag(true);

          if ((_debugLevel > 0) and (ev_counter() < _nEventsDbg)) { 
            TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                             << " subevent:" << i
                             << " EW tag:" << ew_tag
                             << " nbytes: " << nb;
            _dtc_i->PrintBuffer(ev->GetRawBufferPointer(),ev->GetSubEventByteCount()/2);
          }
          rc = 0;
        }
        else {
//-----------------------------------------------------------------------------
// ERROR: read zero bytes
//-----------------------------------------------------------------------------
          TLOG(TLVL_ERROR) << "label:" << _artdaqLabel
                           << " zero length read, event:" << ev_counter();
          message("alarm", _artdaqLabel+"::ReadData::ERROR event="+std::to_string(ev_counter())+" nbytes=0") ;
        }
      }
      
      TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                       << " NDTCs:" << sz
                       << " nbytes:" << nbytes;
    }
    catch (...) {
      TLOG(TLVL_ERROR) << "label:" << _artdaqLabel << "ERROR reading data";
    }
  
  int print_event = (ev_counter() % _printFreq) == 0;
  if (print_event) {
    TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                     << " event:" << ev_counter()
                     << " readSuccess:"  << readSuccess
                     << " timeout:" << timeout << " nbytes:" << nbytes;
  }

  return rc;
}


//-----------------------------------------------------------------------------
bool mu2e::TrackerBRDR::readEvent(artdaq::FragmentPtrs& Frags) {
//-----------------------------------------------------------------------------
// read data
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel << " start";
  _dtc->GetDevice()->ResetDeviceTime();
//-----------------------------------------------------------------------------
// a hack : reduce the PMT logfile size 
//-----------------------------------------------------------------------------
// int print_event = (ev_counter() % _printFreq) == 0;
// make sure even a fake fragment goes in
//-----------------------------------------------------------------------------
  ulong tstamp = CommandableFragmentGenerator::ev_counter();

  if (_readData) {
//-----------------------------------------------------------------------------
// read data 
//-----------------------------------------------------------------------------
    readData(Frags,tstamp);
  }
  else {
//-----------------------------------------------------------------------------
// fake reading
//-----------------------------------------------------------------------------
    artdaq::Fragment* f1 = new artdaq::Fragment(ev_counter(), _fragment_ids[0], _fragmentType, tstamp);
    f1->resizeBytes(4);
    uint* afd  = (uint*) f1->dataBegin();
    *afd = 0x00ffffff;
    Frags.emplace_back(f1);
  }

  TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel << "buffers released, end";
  return true;
}

//-----------------------------------------------------------------------------
bool mu2e::TrackerBRDR::simulateEvent(artdaq::FragmentPtrs& Frags) {

  double tstamp          = ev_counter();
  artdaq::Fragment* frag = new artdaq::Fragment(ev_counter(), _fragment_ids[0], _fragmentType, tstamp);

  const uint16_t fake_event [] = {
    0x01d0 , 0x0000 , 0x0000 , 0x0000 , 0x01c8 , 0x0000 , 0x0169 , 0x0000,   // 0x000000: 
    0x0000 , 0x0101 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0100 , 0x0000,   // 0x000010: 
    0x01b0 , 0x0000 , 0x0169 , 0x0000 , 0x0000 , 0x0101 , 0x0000 , 0x0000,   // 0x000020: 
    0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x01ee,   // 0x000030: 
    0x0190 , 0x8150 , 0x0018 , 0x0169 , 0x0000 , 0x0000 , 0x0155 , 0x0000,   // 0x000040: 
    0x005b , 0x858d , 0x1408 , 0x8560 , 0x0408 , 0x0041 , 0xa955 , 0x155a,   // 0x000050: 
    0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a,   // 0x000060: 
    0x005b , 0x548e , 0x1415 , 0x5462 , 0x0415 , 0x0041 , 0xa955 , 0x155a,   // 0x000070: 
    0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a,   // 0x000080: 
    0x005b , 0x2393 , 0x1422 , 0x2362 , 0x0422 , 0x0041 , 0xa955 , 0x155a,   // 0x000090: 
    0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a,   // 0x0000a0: 
    0x002a , 0x859a , 0x1408 , 0x85b2 , 0x0408 , 0x0041 , 0x56aa , 0x2aa5,   // 0x0000b0: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x0000c0: 
    0x002a , 0x549a , 0x1415 , 0x54b5 , 0x0415 , 0x0041 , 0x56aa , 0x2aa5,   // 0x0000d0: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x0000e0: 
    0x002a , 0x239c , 0x1422 , 0x23b5 , 0x0422 , 0x0041 , 0x56aa , 0x2aa5,   // 0x0000f0: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x000100: 
    0x00de , 0xca6a , 0x1400 , 0xca5c , 0x0400 , 0x0041 , 0x56aa , 0x2aa5,   // 0x000110: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x000120: 
    0x00de , 0x996a , 0x140d , 0x995c , 0x040d , 0x0041 , 0x56aa , 0x2aa5,   // 0x000130: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x000140: 
    0x00de , 0x686c , 0x141a , 0x685d , 0x041a , 0x0041 , 0xa955 , 0x155a,   // 0x000150: 
    0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a,   // 0x000160: 
    0x00ac , 0xc90d , 0x1500 , 0xcabf , 0x0400 , 0x0041 , 0xa955 , 0x155a,   // 0x000170: 
    0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a,   // 0x000180: 
    0x00ac , 0x980d , 0x150d , 0x99c5 , 0x040d , 0x0041 , 0x56aa , 0x2aa5,   // 0x000190: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5,   // 0x0001a0: 
    0x00ac , 0x670d , 0x151a , 0x68c5 , 0x041a , 0x0041 , 0x56aa , 0x2aa5,   // 0x0001b0: 
    0xa955 , 0x155a , 0x56aa , 0x2aa5 , 0xa955 , 0x155a , 0x56aa , 0x2aa5    // 0x0001c0: 
  };

  int nb = 0x1d0;
  frag->resizeBytes(nb);

  uint* afd  = (uint*) frag->dataBegin();
  memcpy(afd,fake_event,nb);

  // printf("%s: fake data\n",__func__);
  // printBuffer(f1->dataBegin(),4);

  Frags.emplace_back(frag);
  return true;
}

//-----------------------------------------------------------------------------
// a virtual function called from the outside world
//-----------------------------------------------------------------------------
bool mu2e::TrackerBRDR::getNext_(artdaq::FragmentPtrs& Frags) {
  bool rc(true);

  TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                   << " event:" << ev_counter() << " STARTING";
//-----------------------------------------------------------------------------
// in the beginning, send message to the Farm manager
//-----------------------------------------------------------------------------
  if (ev_counter() == 1) {
    std::string msg = "TrackerBRDR::getNext: " + std::to_string(ev_counter());
    message("info",msg);
  }

  if (should_stop()) return false;

  _startProcTimer();

  TLOG(TLVL_DEBUG) << "label:" << _artdaqLabel
                   << " event:" << ev_counter()
                   << " after startProcTimer";

  if (_readoutMode < 100) {
//-----------------------------------------------------------------------------
// attempt to read data
//-----------------------------------------------------------------------------
    rc = readEvent(Frags);
  }
  else {
//-----------------------------------------------------------------------------
//    readout mode > 100 : simulate event internally
//-----------------------------------------------------------------------------
    rc = simulateEvent(Frags);
  }
//-----------------------------------------------------------------------------
// increment number of generated events
//-----------------------------------------------------------------------------
  CommandableFragmentGenerator::ev_counter_inc();
//-----------------------------------------------------------------------------
// if needed, increment the subrun number
//-----------------------------------------------------------------------------
  if ((CommandableFragmentGenerator::ev_counter() % _maxEventsPerSubrun) == 0) {

    artdaq::Fragment* esf = new artdaq::Fragment(1);
    esf->setSystemType(artdaq::Fragment::EndOfSubrunFragmentType);

    long int ew_tag = ev_counter();
    esf->setSequenceID(ew_tag+1);
//-----------------------------------------------------------------------------
// not sure I understand the logic of assigning the first event number in the subrun here
//-----------------------------------------------------------------------------
    esf->setTimestamp(1 + (ew_tag / _maxEventsPerSubrun));
    *esf->dataBegin() = 0;
    Frags.emplace_back(esf);
  }

  return rc;
}

//-----------------------------------------------------------------------------
bool mu2e::TrackerBRDR::sendEmpty_(artdaq::FragmentPtrs& Frags) {
  Frags.emplace_back(new artdaq::Fragment());
  Frags.back()->setSystemType(artdaq::Fragment::EmptyFragmentType);
  Frags.back()->setSequenceID(ev_counter());
  Frags.back()->setFragmentID(_fragment_ids[0]);
  ev_counter_inc();
  return true;
}


// The following macro is defined in artdaq's GeneratorMacros.hh header
DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(mu2e::TrackerBRDR)
