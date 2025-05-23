///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
#include "artdaq/DAQdata/Globals.hh"
#define TRACE_NAME "TrackerBRDR"
#include "TString.h"
#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Data/MetadataFragment.hh"
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

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "midas.h"
#include "utils/utils.hh"
#include "utils/OdbInterface.hh"

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

    struct RocDataHeaderPacket_t {            // 8 16-byte words in total
                                        // 16-bit word 0
      uint16_t            byteCount    : 16;
                                        // 16-bit word 1
      uint16_t            unused       : 4;
      uint16_t            packetType   : 4;
      uint16_t            linkID       : 3;
      uint16_t            DtcErrors    : 4;
      uint16_t            valid        : 1;
                                        // 16-bit word 2
      uint16_t            packetCount  : 11;
      uint16_t            unused2      : 2;
      uint16_t            subsystemID  : 3;
                                        // 16-bit words 3-5
      uint16_t            eventTag[3];
                                        // 16-bit word 6
      uint8_t             status       : 8;
      uint8_t             version      : 8;
                                        // 16-bit word 7
      uint8_t             dtcID        : 8;
      uint8_t             onSpill      : 1;
      uint8_t             subrun       : 2;
      uint8_t             eventMode    : 5;
                                        // decoding status

      int                 empty     () { return (status & 0x01) == 0; }
      int                 invalid_dr() { return (status & 0x02); }
      int                 corrupt   () { return (status & 0x04); }
      int                 timeout   () { return (status & 0x08); }
      int                 overflow  () { return (status & 0x10); }

      int                 error_code() { return (status & 0x1e); }
    };

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
    int                                   _pcieAddr;
    int                                   _linkMask;           // from ODB

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
    std::string                           _xmlrpcUrl;          // 
    xmlrpc_env                            _env;                // XML-RPC environment
    ulong                                 _tstamp;             //

    std::string                           _midas_host;
    std::string                           _exptName;
    std::string                           _host_label;
    std::string                           _full_host_name;
    std::string                           _tfmHost;            // used to send xmlrpc messages to
    int                                   _partition_id;

    HNDLE                                 _hBoardreader;       // boardreader handle in ODB, hopefully doesn't change
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
    //    void printBuffer        (const void* ptr, int sz);
//-----------------------------------------------------------------------------
// try follow Simon ... perhaps one can improve on bool? 
// also do not pass strings by value
//-----------------------------------------------------------------------------
    int  message(const std::string& msg_type, const std::string& message);
                                        // read functions
    int  readData        (artdaq::FragmentPtrs& Frags, ulong& Timestamp);

    int  validateFragment(void* DtcFragment);

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
  , _sFragmentType     (ps.get<std::string>             ("fragmentType"       ,    "DTCEVT"))  // 
  , _debugLevel        (ps.get<int>                     ("debugLevel"         ,           0))
  , _nEventsDbg        (ps.get<size_t>                  ("nEventsDbg"         ,         100))
  , _readData          (ps.get<int>                     ("readData"           ,           1))  // 
  , _printFreq         (ps.get<int>                     ("printFreq"          ,         100))  // 
  , _maxEventsPerSubrun(ps.get<int>                     ("maxEventsPerSubrun" ,       10000))  // 
  , _readoutMode       (ps.get<int>                     ("readoutMode"        ,           1))  // 
  
{
  _fragmentType = mu2e::toFragmentType(_sFragmentType);
  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " CONSTRUCTOR (1) readData:" << _readData
                     << " fragmentType:" << _sFragmentType << ":" << int(_fragmentType);
//-----------------------------------------------------------------------------
// the BR interface should not be changing any settings, just read events
// DTC is already initialized by the frontend, don't change anything !
// for skip_init=true, the linkmask is not used. The board reader shouldn't even know
// about it, but it has to know about the DTC PCIE address
// figure out the PCIE address - nothing wrong with connecting to ODB and
//-----------------------------------------------------------------------------
  _midas_host = getenv("MIDAS_SERVER_HOST");
  
  char host_name[100], exp_name[100];
  cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));
  _exptName = exp_name;

  int status = cm_connect_experiment(_midas_host.data(), _exptName.data(), _artdaqLabel.data(),NULL);
  if (status != CM_SUCCESS) {
    cm_msg(MERROR, _artdaqLabel.data(),
           "Cannot connect to experiment \'%s\' on host \'%s\', status %d",
           exp_name,_midas_host.data(),status);
    TLOG(TLVL_ERROR) << "label: " << _artdaqLabel
                     << " ERROR: failed to connect to MIDAS on host:" << _midas_host << " . BAIL OUT";
    /* let user read message before window might close */
    ss_sleep(5000);
    return;
  }

  HNDLE  hDB(0);
  HNDLE  hClient(0);
  cm_get_experiment_database(&hDB, &hClient);

  OdbInterface* odb_i         = OdbInterface::Instance(hDB);
  HNDLE h_active_run_conf     = odb_i->GetActiveRunConfigHandle();
  std::string active_run_conf = odb_i->GetRunConfigName(h_active_run_conf);
//-----------------------------------------------------------------------------
// TFM host name - on the private network
//-----------------------------------------------------------------------------
  _tfmHost                    = odb_i->GetTfmHostName  (h_active_run_conf);
  _partition_id               = odb_i->GetPartitionID  (h_active_run_conf);

  std::string private_subnet  = odb_i->GetPrivateSubnet(h_active_run_conf);
  std::string public_subnet   = odb_i->GetPublicSubnet (h_active_run_conf);

  _host_label                 = get_short_host_name(public_subnet.data() );
  _full_host_name             = get_full_host_name (private_subnet.data());

  HNDLE h_host_artdaq_conf    = odb_i->GetHostArtdaqConfHandle(h_active_run_conf,_host_label);

  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " _full_host_name:" << _full_host_name
                     << " , exp_name: " << exp_name
                     << " TRACE_FILE:" << (getenv("TRACE_FILE")) ? getenv("TRACE_FILE") : "undefined";

  
  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " hDB:" << hDB << " hClient:" <<  hClient;

  if (hDB == 0) {
    TLOG(TLVL_ERROR) << "label:" << _artdaqLabel << " ERROR: failed to connect to ODB. BAIL OUT";
    return;
  }


  TLOG(TLVL_DEBUG+1) << "label: " << _artdaqLabel
                     << " h_active_run_conf:" << h_active_run_conf
                     << " active_run_conf_name:" << active_run_conf
                     << " _full_host_name:" << _full_host_name
                     << " h_host_artdaq_conf:" << h_host_artdaq_conf;
//-----------------------------------------------------------------------------
// a DAQ host has keys: "DTC0", DTC1", and "Artdaq"
//-----------------------------------------------------------------------------
//  HNDLE h_component;
//  KEY   component;

  _hBoardreader = odb_i->GetHandle(h_host_artdaq_conf,_artdaqLabel.data());
  HNDLE hdtc    = odb_i->GetHandle(_hBoardreader,"DTC");
  _pcieAddr     = odb_i->GetDtcPcieAddress(hdtc);
  _linkMask     = odb_i->GetLinkMask      (hdtc);

  TLOG(TLVL_DEBUG+1) << "label:"             << _artdaqLabel
                     << " pcie_addr(ODB):"   << _pcieAddr
                     << " link mask(ODB):0x" << std::hex << _linkMask;
  
  TLOG(TLVL_DEBUG+1) << "label:"               << _artdaqLabel
                     << " active_run_conf:"    << active_run_conf
                     << " _tfmHost_from_ODB:"  << _tfmHost;
  
  cm_disconnect_experiment();
//-----------------------------------------------------------------------------
// boardreaders do not re-initialize the DTCs
// make sure that the link mask is right no matter what
//-----------------------------------------------------------------------------
  bool skip_init(true);
  _dtc_i = trkdaq::DtcInterface::Instance(_pcieAddr,_linkMask,skip_init);
  _dtc   = _dtc_i->Dtc();
//-----------------------------------------------------------------------------
// print status of the DTC registers
// don't touch DCS ! 
//-----------------------------------------------------------------------------
  _dtc_i->PrintStatus();
  //  _dtc_i->PrintRocStatus();
//-----------------------------------------------------------------------------
// finally, initialize the environment for the XML-RPC messaging client
// use ARTDAQ port numbering convention
//-----------------------------------------------------------------------------
  int rpc_port = 10000+1000*_partition_id;
  _xmlrpcUrl   = "http://" + _tfmHost + ":" + std::to_string(rpc_port)+"/RPC2";
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "debug", "v1_0");
  xmlrpc_env_init(&_env);

  _tstamp           = 0;
}

//-----------------------------------------------------------------------------
// let the boardreader send messages back to the TFM and report problems 
// so far, make the TFM hist a talk-to parameter
// GetPartitionNumber() is an artdaq global function - see artdaq/artdaq/DAQdata/Globals.hh
//-----------------------------------------------------------------------------
int mu2e::TrackerBRDR::message(const std::string& MsgType, const std::string& Msg) {
    
  xmlrpc_client_call(&_env, _xmlrpcUrl.data(), "message","(ss)", MsgType.data(), 
                     (artdaq::Globals::app_name_+":"+Msg).data());
  if (_env.fault_occurred) {
    TLOG(TLVL_ERROR) << "label:" << _artdaqLabel << " event:" << ev_counter() << " XML-RPC rc=" << _env.fault_code << " " << _env.fault_string;
    return -1;
  }
  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " event:" << ev_counter() << "message sent. type:" << MsgType << " message:" << Msg;
  return 0;
}

//-----------------------------------------------------------------------------
mu2e::TrackerBRDR::~TrackerBRDR() {
//-----------------------------------------------------------------------------
// print status of the DTC registers in the end of the run,
// but don't try DCS - the DTC PCIE driver is claimed to be a uniclient facility....
//-----------------------------------------------------------------------------
  _dtc_i->PrintStatus();
  //   _dtc_i->PrintRocStatus();
} 

//-----------------------------------------------------------------------------
void mu2e::TrackerBRDR::stop() {
}

//-----------------------------------------------------------------------------
int mu2e::TrackerBRDR::validateFragment(void* ArtdaqFragmentData) {
//-----------------------------------------------------------------------------
// quick check of the data - at ROC level
// DTC header: 3 packets, followed by the ROC data
// there should be 6 ROC blocks, empty ROC - 0x10 bytes
//-----------------------------------------------------------------------------
  uint16_t* dat = (uint16_t*) ArtdaqFragmentData;
  //  int       nb  = *dat;
  
  uint16_t* roc = dat+0x18;             // here the first ROC starts
  
  int       nerr = 0;
  uint32_t  err_code(0);
  int       evt_nb(0), roc_nb[6];

  TLOG(TLVL_DEBUG+1) << "START";
  for (int i=0; i<6; i++) {
    RocDataHeaderPacket_t* rdh = (RocDataHeaderPacket_t*) roc;
    roc_nb[i] = rdh->byteCount;
    if (_dtc_i->LinkEnabled(i)) {
      if (rdh->error_code()) {
//-----------------------------------------------------------------------------
// some kind of an error 
//-----------------------------------------------------------------------------
        err_code |= 0x1;
        if (rdh->error_code() == 0x10) {
//-----------------------------------------------------------------------------
// buffer overflow - make it a warning
//-----------------------------------------------------------------------------
          std::string msg = std::format("event:{} link:{} ERROR CODE:{:#04x} NBYTES:{}",
                                        ev_counter(),i,rdh->error_code(),roc_nb[i]);
          cm_msg(MINFO, _artdaqLabel.data(),msg.data());
          TLOG(TLVL_WARNING) << _artdaqLabel.data() << ": " << msg;
        }
        else if (rdh->error_code() == 0x8) {
//-----------------------------------------------------------------------------
// timeout - definitely an error
//-----------------------------------------------------------------------------
          nerr += 1;
          std::string msg = std::format("event:{} link:{} ERROR CODE:{:#04x} NBYTES:{}",
                                        ev_counter(),i,rdh->error_code(),roc_nb[i]);
          cm_msg(MERROR, _artdaqLabel.data(),msg.data());
          TLOG(TLVL_ERROR) << _artdaqLabel.data() << ": " << msg;
        }
      }
    }
    else {
//-----------------------------------------------------------------------------
// link is supposed to be disabled
//-----------------------------------------------------------------------------
      if (roc_nb[i] != 0x10) {
        err_code |= 0x2;
        nerr     += 1;
        std::string msg = std::format("event:{:10d} non-zero payload for DISABLED link:{}",ev_counter(),i);
        cm_msg(MERROR, _artdaqLabel.data(),msg.data());
        TLOG(TLVL_ERROR) << _artdaqLabel.data() << ": " << msg;
      }
    }
    evt_nb += roc_nb[i];
    roc    += roc_nb[i]/2;
  }

  TLOG(TLVL_DEBUG+1) << "event:" << ev_counter() << " nerr:" << nerr
                     << " err_code:0x" << std::hex << err_code
                     << std::format("roc_nb: {:5}{:5}{:5}{:5}{:5}{:5}",
                                    roc_nb[0],roc_nb[1],roc_nb[2],roc_nb[3],roc_nb[4],roc_nb[5]);
  
  if (nerr != 0) {
//-----------------------------------------------------------------------------
// send alarm message and set the boardreader status to -1, also log the message
//-----------------------------------------------------------------------------
// 2025-04-19 PM?    int status = cm_connect_experiment(_midas_host.data(), _exptName.data(), _artdaqLabel.data(),NULL);
// 2025-04-19 PM?    if (status != CM_SUCCESS) {
    
    // cm_msg(MERROR, _artdaqLabel.data(),
    //        "Cannot connect to experiment '%s' on host '%s' from '%s', status %d",
    //        _exptName.data(),_midas_host.data(),_full_host_name.data(),status);

    cm_msg(MERROR, _artdaqLabel.data(),
           "Cannot connect to experiment '%s' on host '%s' from '%s'",
           _exptName.data(),_midas_host.data(),_full_host_name.data());
    
      /* let user read message before window might close */
    //   ss_sleep(5000);
    //   return -1;
    // }
    
// 2025-04-19 PM?    HNDLE  hdb(0), hClient(0);
// 2025-04-19 PM?    cm_get_experiment_database(&hdb, &hClient);
// 2025-04-19 PM?    OdbInterface* odb_i = OdbInterface::Instance(hdb);
// 2025-04-19 PM?    odb_i->SetStatus(_hBoardreader,-1);
// 2025-04-19 PM?    cm_disconnect_experiment();
  }
  
  TLOG(TLVL_DEBUG+1) << "event:" << ev_counter() << " END";
  return 0;
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

  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
                     << " event:" << ev_counter() 
                     << " START TStamp:" << TStamp << std::endl;

  DTC_EventWindowTag event_tag = DTC_EventWindowTag(_tstamp);

  try {
//------------------------------------------------------------------------------
// sz: 1 (or 0, if nothing has been read out)
//     so far, get a single (DTC) subevent
//-----------------------------------------------------------------------------
      subevents = _dtc->GetSubEventData(event_tag,match_ts);
      int sz    = subevents.size();
      TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
                         << " event:" << ev_counter()
                         << " read:" << sz
                         << " DTC blocks" << std::endl;
      if (sz > 0) {
        _tstamp  += 1;
      }
      else {
                                        // ERROR
        cm_msg(MERROR, _artdaqLabel.data(),Form("event: %10li subevents.size():%i",ev_counter(),sz));
        TLOG(TLVL_ERROR) << "event:" << ev_counter() << " subevents.size():" << sz;
        rc = 0;
        return rc;
      }
//-----------------------------------------------------------------------------
// each subevent (a block of data corresponding to a single DTC) becomes an artdaq fragment
//-----------------------------------------------------------------------------
      for (int i=0; i<sz; i++) {                       // and so far sz = 1
        DTC_SubEvent* ev     = subevents[i].get();
        int           nb     = ev->GetSubEventByteCount();
        uint64_t      ew_tag = ev->GetEventWindowTag().GetEventWindowTag(true);

        TStamp = ew_tag;  // hack ?? may be not
        
        TLOG(TLVL_DEBUG+1) << "label:"      << _artdaqLabel
                           << " dtc_block:" << i
                           << " nbytes:" << nb << std::endl;
        nbytes += nb;
        if (nb > 0) {
          artdaq::Fragment* frag = new artdaq::Fragment(ev_counter(), _fragment_ids[0], FragmentType::DTCEVT, TStamp);
          int event_size = nb+sizeof(DTCLib::DTC_EventHeader);
          frag->resizeBytes(event_size);
      
          DTCLib::DTC_EventHeader* hdr = (DTCLib::DTC_EventHeader*) frag->dataBegin();
          hdr->inclusive_event_byte_count = event_size;
          hdr->num_dtcs       = 1;
          hdr->event_tag_low  = (TStamp      ) & 0xFFFFFFFF;
          hdr->event_tag_high = (TStamp >> 32) & 0x0000FFFF;

          void* afd  = (void*) (hdr+1);
          memcpy(afd,ev->GetRawBufferPointer(),nb);

          validateFragment(afd);        // skip validation of the header
//-----------------------------------------------------------------------------
// now copy the fragment
//-----------------------------------------------------------------------------
          Frags.emplace_back(frag);
//-----------------------------------------------------------------------------
// this is essentially it, now - diagnostics 
//-----------------------------------------------------------------------------
          uint64_t ew_tag = ev->GetEventWindowTag().GetEventWindowTag(true);

          if ((_debugLevel > 0) and (ev_counter() < _nEventsDbg)) { 
            TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
                               << " subevent:" << i
                               << " EW tag:" << ew_tag
                               << " nbytes: " << nb;
            _dtc_i->PrintBuffer(ev->GetRawBufferPointer(),ev->GetSubEventByteCount()/2,&std::cout);
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
      
      TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
                         << " NDTCs:" << sz
                         << " nbytes:" << nbytes;
    }
    catch (...) {
      TLOG(TLVL_ERROR) << "label:" << _artdaqLabel << "ERROR reading data";
    }
  
  int print_event = (ev_counter() % _printFreq) == 0;
  if (print_event) {
    TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
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
  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " start";
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
    int rc = readData(Frags,tstamp);
    if (rc < 0) {
      TLOG(TLVL_ERROR) << "label:" << _artdaqLabel << " rc(readData):" << rc;
    }
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

  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " buffers released, end";
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

  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " event:" << ev_counter() << " START";
//-----------------------------------------------------------------------------
// in the beginning, send message to the Farm manager
//-----------------------------------------------------------------------------
  if (ev_counter() == 1) {
    std::string msg = "TrackerBRDR::getNext: " + std::to_string(ev_counter());
    message("info",msg);
  }

  if (should_stop()) return false;

  _startProcTimer();

  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel
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

    auto endOfSubrunFrag = artdaq::MetadataFragment::CreateEndOfSubrunFragment(artdaq::Globals::my_rank_, ev_counter() + 1, 1 + (ev_counter() / _maxEventsPerSubrun), 0);
    Frags.emplace_back(std::move(endOfSubrunFrag));
  }
  
  TLOG(TLVL_DEBUG+1) << "label:" << _artdaqLabel << " event:" << ev_counter() << " END";

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
