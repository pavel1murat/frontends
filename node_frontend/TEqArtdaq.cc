///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include <format>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"

#include "node_frontend/ArtdaqMetrics.hh"
#include "node_frontend/TEquipmentManager.hh"
#include "node_frontend/TEqArtdaq.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

// #include "xmlrpc-c/config.h"  /* information about this build environment */
// #include <xmlrpc-c/base.h>
// #include <xmlrpc-c/client.h>

using namespace std;
using nlohmann::json;

#include <boost/algorithm/string.hpp>

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEqArtdaq"

std::initializer_list<const char*> BrVarName = {
  "nf_read", "gn_rate", "f_rate" , "inp_mw", "min_nf",                                //  0
  "max_nf" , "etime"  , "nf_sent", "out_fr", "out_dr",                                //  5
  "outp_mw", "min_evt_size", "max_evt_size", "inp_wait_time", "buf_wait_time",        // 10
  "req_wait_time", "out_wait_time",                                                   // 15
  "fid[0]"   ,"fid[1]"   ,"fid[2]"   ,"fid[3]"   ,"fid[4]"   ,                        // 17
  "nf_shm[0]","nf_shm[1]","nf_shm[2]","nf_shm[3]","nf_shm[4]",                        // 22
  "nb_shm[0]","nb_shm[1]","nb_shm[2]","nb_shm[3]","nb_shm[4]"                         // 27
};

std::initializer_list<const char*> EbVarName = {
  "nev_read"    , "evt_rate", "data_rate" , "mon_win", "min_evt_size",                //  0
  "max_evt_size", "etime"  , "nfr_read", "nfr_per_sec", "fr_dr",                      //  5
  "min_fr_size", "max_fr_size", "nev_tot_rn", "nev_bad_rn", "nev_tot_sr",             // 10
  "nev_bad_sr", "nbuf_shm_tot", "nbytes_shm_tot", "nbuf_shm_empty", "nbuf_shm_write", // 15
   "nbuf_shm_full","nbuf_shm_read"                                                    // 20
};

std::initializer_list<const char*>  DlVarName = {
  "nev_read"    , "evt_rate", "data_rate" , "mon_win", "min_evt_size",                //  0
  "max_evt_size", "etime"  , "nfr_read", "nfr_per_sec", "fr_dr",                      //  5
  "min_fr_size", "max_fr_size", "nev_tot_rn", "nev_bad_rn", "nev_tot_sr",             // 10
  "nev_bad_sr", "nbuf_shm_tot", "nbytes_shm_tot", "nbuf_shm_empty", "nbuf_shm_write", // 15
   "nbuf_shm_full","nbuf_shm_read"                                                    // 20
};

std::initializer_list<const char*>  DsVarName = {
};


#include "node_frontend/TEqArtdaq.hh"

//-----------------------------------------------------------------------------
TEqArtdaq::TEqArtdaq(const char* EqName) : TMu2eEqBase(EqName) {
//-----------------------------------------------------------------------------
// get port number used by the TFM, don't assume the farm_manager is running locally
// the frontend has to have its own xmlrpc URL,
// TFM uses port           10000+1000*partition
// boardreaders start from 10000+1000*partition+100+1;
// init XML RPC            10000+1000*partition+11
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << std::format("-- START _host_label:{}",_host_label);
//-----------------------------------------------------------------------------
// read ARTDAQ configuration from ODB
//-----------------------------------------------------------------------------
  HNDLE hdb           = _odb_i->GetDbHandle();
  HNDLE h_artdaq_conf = _odb_i->GetHostArtdaqConfHandle(_h_active_run_conf,_host_label);
  HNDLE h_component;
  KEY   component;
  for (int i=0; db_enum_key(hdb, h_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
//-----------------------------------------------------------------------------
// use the component label 
// component names:
//                   brxx - board readers
//                   ebxx - event builders
//                   dlxx - data loggers
//                   dsxx - dispatchers
//-----------------------------------------------------------------------------
    db_get_key(hdb, h_component, &component);
    TLOG(TLVL_DEBUG) << "i:" << i
                     << " component.name:" << component.name
                     << " component.type:" << component.type;
//------------------------------------------------------------------------------
// "Artdaq" can also be disabled - on a node ? or want a global flag ?
// stay with local for now...
//-----------------------------------------------------------------------------
    if ((strcmp(component.name,"Enabled") == 0) or (strcmp(component.name,"Status") == 0)) continue;

    ArtdaqComponent_t ac;

    _odb_i->GetInteger(h_component,"Enabled"    ,&ac.enabled);
    _odb_i->GetInteger(h_component,"Status"     ,&ac.status);

    if ((ac.enabled != 1) || (ac.status != 0)) {
      TLOG(TLVL_WARNING) << " ARTDAQ component:" << component.name
                         << " enabled:"          << ac.enabled
                         << " status:"           << ac.status
                         << " : NOT INITIALIZED";
                                                            continue;
    }
    
    ac.name        = component.name;

    if      (ac.name.find("br") == 0) ac.type = kBoardReader;
    else if (ac.name.find("eb") == 0) ac.type = kEventBuilder;
    else if (ac.name.find("dl") == 0) ac.type = kDataLogger;
    else if (ac.name.find("ds") == 0) ac.type = kDispatcher;

    int x;
    _odb_i->GetInteger(h_component,"XmlrpcPort",&x);
    ac.xmlrpc_port = std::format("{}",x);
    
    ac.subsystem   = _odb_i->GetString (h_component,"Subsystem");

    _odb_i->GetInteger(h_component,"Rank"          ,&ac.rank);
    // _odb_i->GetInteger(hDB,h_component,"Target"    ,&ac.target);
    _odb_i->GetInteger(h_component,"NFragmentTypes",&ac.n_fragment_types);

    // char url[100];
    // sprintf(url,"http://%s:%i/RPC2",_full_host_name.data(),ac.xmlprc_port);
    ac.xmlrpc_url  = std::format("http://{}:{}/RPC2",_full_host_name,ac.xmlrpc_port);
    
    _list_of_ac.push_back(ac);
  }
  
  _monitoringLevel = _odb_i->GetInteger(_h_daq_host_conf,"Monitor/Artdaq");

  TLOG(TLVL_DEBUG) << std::format("_monitoringLevel:{}",_monitoringLevel);

  InitVarNames();
  
//-----------------------------------------------------------------------------
// hotlinks - start from one function handling both DTCs
// command processor : 'ProcessCommand' function
//-----------------------------------------------------------------------------
  // HNDLE hdb            = _odb_i->GetDbHandle();
  std::string cmd_path = _odb_i->GetCmdConfigPath(HostLabel(),_name);
  HNDLE h_cmd          = _odb_i->GetHandle(0,cmd_path.data());
  HNDLE h_cmd_run      = _odb_i->GetHandle(h_cmd,"Run");
  
  TLOG(TLVL_DEBUG) << std::format("before db_open_record: cmd_path:{} h_cmd:{} h_cmd_run:{}",cmd_path,h_cmd,h_cmd_run);
    
  if (db_open_record(hdb,h_cmd_run,&_cmd_run,sizeof(int32_t),MODE_READ,ProcessCommand, NULL) != DB_SUCCESS)  {
    std::string m = std::format("cannot open ARTDAQ hotlink in ODB");
    cm_msg(MERROR, __func__,m.data());
    TLOG(TLVL_ERROR) << m;
  }

  std::string data_dir = _odb_i->GetString(0,"/Logger/Data dir");
  _logfile             = std::format("{}/artdaq.log",data_dir);
  
  TLOG(TLVL_DEBUG) << "-- END";
}

//-----------------------------------------------------------------------------
TEqArtdaq::~TEqArtdaq() {
}

//-----------------------------------------------------------------------------
TMFeResult TEqArtdaq::Init() {
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// boardreaders: 'br01', 'br02' , etc
//-----------------------------------------------------------------------------
int TEqArtdaq::InitVarNames() {
  char dirname[128], name[128];

  TLOG(TLVL_DEBUG) << "START";
  const std::string node_odb_path = "/Equipment/"+HostLabel();
  const std::string settings_path = node_odb_path+"/Settings";
  midas::odb        odb_settings(settings_path);
//-----------------------------------------------------------------------------
  int nac = _list_of_ac.size();
  
  for (int i=0; i<nac; i++) {
    ArtdaqComponent_t* ac = &_list_of_ac[i];

    std::vector<std::string> var_names;
    if (ac->type == kBoardReader) {
      for (int k=0; k<NBrDataWords; k++) {
        sprintf(name,"%s#%s",ac->name.data(),BrVarName.begin()[k]);
        var_names.push_back(name);
      }
    }
    else if (ac->type == kEventBuilder) {
      for (int k=0; k<NEbDataWords; k++) {
        sprintf(name,"%s#%s",ac->name.data(),EbVarName.begin()[k]);
        var_names.push_back(name);
      }
    }
    else if (ac->type == kDataLogger) {
      for (int k=0; k<NDlDataWords; k++) {
        sprintf(name,"%s#%s",ac->name.data(),DlVarName.begin()[k]);
        var_names.push_back(name);
      }
    }
    else if (ac->type == kDispatcher) {
      for (int k=0; k<NDsDataWords; k++) {
        sprintf(name,"%s#%s",ac->name.data(),DsVarName.begin()[k]);
        var_names.push_back(name);
      }
    }
//-----------------------------------------------------------------------------
// save in ODB
//-----------------------------------------------------------------------------
    sprintf(dirname,"Names %s",ac->name.data());
    char path[256];
    sprintf(path,"%s/%s",settings_path.data(),dirname);
    if ((var_names.size() > 0) and (not midas::odb::exists(path))) {
      odb_settings[dirname] = var_names;
    }
  }
  TLOG(TLVL_DEBUG) << "END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqArtdaq::ReadBrMetrics(const ArtdaqComponent_t* Ac) {
  int rc(0);
  TLOG(TLVL_DEBUG+1) << "-- START, running:" << TMFE::Instance()->fStateRunning;

  std::regex pattern(R"(/RPC2$)");
  std::string url = std::regex_replace(Ac->xmlrpc_url, pattern, "");

  std::string cmd = std::format("python config/scripts/artdaq_xmlrpc.py --test=br_metrics --url={}",url);

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  if (output[0] != '{') {
    TLOG(TLVL_ERROR) << "wrong output:'" << output << "'";
    return -1;
  }

  TEquipmentManager* eqm = TEquipmentManager::Instance();

  try {
    json brm = json::parse(output);
    TLOG(TLVL_DEBUG+1) << "parsed json:" << brm << std::endl;

    char buf[1024];
  
    eqm->ComposeEvent(buf, sizeof(buf));
    eqm->BkInit      (buf, sizeof(buf));
    double* ptr = (double*) eqm->BkOpen(buf, Ac->name.data(), TID_DOUBLE);
     
    *ptr++ = brm["nf_sent_tot"];        // 
    *ptr++ = brm["gn_rate"];
    *ptr++ = brm["fr_rate"];
    *ptr++ = brm["time_win"];
    *ptr++ = brm["min_nf"];
    *ptr++ = brm["max_nf"];
    *ptr++ = brm["elapsed_time"];
    
    TLOG(TLVL_DEBUG+1) << "repacking 0.5";
    
    *ptr++ = brm["nf_sent"];
    *ptr++ = brm["of_rate"];
    *ptr++ = brm["dt_rate"];
    *ptr++ = brm["time_win2"];
    *ptr++ = brm["min_ev_size"];
    *ptr++ = brm["max_ev_size"];
    *ptr++ = brm["inp_wait_time"];
    *ptr++ = brm["buf_wait_time"];
    *ptr++ = brm["req_wait_time"];
    *ptr++ = brm["out_wait_time"];

    TLOG(TLVL_DEBUG+1) << "repacking 1";
//-----------------------------------------------------------------------------
// shared memory
//-----------------------------------------------------------------------------
    double* psave = ptr;
    int nf = brm["fids"].size();
    TLOG(TLVL_DEBUG+1) << "nf:" << nf << " brm[\"fids\"]:" << brm["fids"];
    
    for (int i=0; i<nf; i++) {
      *ptr      = brm["fids"][i]["fid"];
      *(ptr+5)  = brm["fids"][i]["nf" ];
      *(ptr+10) = brm["fids"][i]["nb" ];
      ptr++;
    }
    TLOG(TLVL_DEBUG+1) << "repacking 2";
//-----------------------------------------------------------------------------
// record has a fixed length !
//-----------------------------------------------------------------------------
    ptr = psave+15;
    eqm->BkClose    (buf,ptr);
    eqm->EqSendEvent(buf);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "json::parse failed to parse output" ;
  }

//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG+1) << "-- END, rc:" << rc;
 
  return rc;
}

/* ------------------------------------------------------------------------------
 eb01 statistics:
  Event statistics: 20200 events released at 336.642 events/sec, effective data rate = 6.14159 MB/sec, monitor window = 60.0045 sec, min::max event size = 0.0150223::0.0223465 MB
  Average time per event:  elapsed time = 0.00297052 sec
  Fragment statistics: 40400 fragments received at 673.283 fragments/sec, effective data rate = 6.12873 MB/sec, monitor window = 60.0045 sec, min::max fragment size = 0.00749207::0.0118866 MB
  Event counts: Run -- 30400 Total, 4 Incomplete.  Subrun -- 0 Total, 0 Incomplete. 
shm_nbb :250:2306872:246:4:0:0
 ------------------------------------------------------------------------------ */

//-----------------------------------------------------------------------------
// 'Dr' stands for 'DataReceiver' which means 'event builder' or 'data logger'
//-----------------------------------------------------------------------------
int TEqArtdaq::ReadDrMetrics(const ArtdaqComponent_t* Ac) {
  int rc(0);
  
  TLOG(TLVL_DEBUG+1) << "-- START, running:" << TMFE::Instance()->fStateRunning;

  std::regex pattern(R"(/RPC2$)");
  std::string url = std::regex_replace(Ac->xmlrpc_url, pattern, "");

  std::string cmd = std::format("python config/scripts/artdaq_xmlrpc.py --test=dr_metrics --url={}",url);

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  if (output[0] != '{') {
    TLOG(TLVL_ERROR) << "wrong output:'" << output << "'";
    return -1;
  }

  json drm = json::parse(output);

  TLOG(TLVL_DEBUG+1) << "parsed json:" << drm;
//-----------------------------------------------------------------------------
// copy data to the output
//-----------------------------------------------------------------------------
  char buf[1024];
  
  TEquipmentManager* eqm = TEquipmentManager::Instance();

  eqm->ComposeEvent(buf, sizeof(buf));
  eqm->BkInit      (buf, sizeof(buf));
  float* ptr = (float*) eqm->BkOpen(buf, Ac->name.data(), TID_FLOAT);
     
  *ptr++ = drm["nev_read"    ];        // 
  *ptr++ = drm["evt_rate"    ];
  *ptr++ = drm["data_rate"   ];
  *ptr++ = drm["time_window" ];
  *ptr++ = drm["min_evt_size"];
  *ptr++ = drm["max_evt_size"];
  *ptr++ = drm["etime"       ];

  TLOG(TLVL_DEBUG+1) << "checkpoint 1";

  *ptr++ = drm["nfr_read"    ];
  *ptr++ = drm["nfr_rate"    ];
  *ptr++ = drm["fr_data_rate"];
  *ptr++ = drm["min_fr_size" ];
  *ptr++ = drm["max_fr_size" ];
  
  TLOG(TLVL_DEBUG+1) << "checkpoint 2";

  *ptr++ = drm["nev_tot_rn"];
  *ptr++ = drm["nev_bad_rn"];
  *ptr++ = drm["nev_tot_sr"];
  *ptr++ = drm["nev_bad_sr"];

  TLOG(TLVL_DEBUG+1) << "checkpoint 3";
  
  *ptr++ = drm["nbuf_shm_tot"  ];
  *ptr++ = drm["nbytes_shm_tot"];
  *ptr++ = drm["nbuf_shm_empty"];
  *ptr++ = drm["nbuf_shm_write"];
  *ptr++ = drm["nbuf_shm_full" ];
  *ptr++ = drm["nbuf_shm_read" ];
  
  TLOG(TLVL_DEBUG+1) << "before closing the bank";

  eqm->BkClose    (buf,ptr);
  eqm->EqSendEvent(buf);

  TLOG(TLVL_DEBUG+1) << "-- END, rc:" << rc;

  return rc;
}



//-----------------------------------------------------------------------------
int TEqArtdaq::ReadDsMetrics(const ArtdaqComponent_t* Ac) {
  return 0;
}
  

//-----------------------------------------------------------------------------
int TEqArtdaq::ReadMetrics() {

  TLOG(TLVL_DEBUG+1) << "-- START";

  int nbr = _list_of_ac.size();
  for (int i=0; i<nbr; i++) {
    ArtdaqComponent_t* ac = &_list_of_ac[i];
    if      (ac->type == kBoardReader ) ReadBrMetrics(ac);
    else if (ac->type == kEventBuilder) ReadDrMetrics(ac);
    else if (ac->type == kDataLogger  ) ReadDrMetrics(ac);
    else if (ac->type == kDispatcher  ) ReadDsMetrics(ac);
  }

  TLOG(TLVL_DEBUG+1) << "-- END";
  return 0;
}

//-----------------------------------------------------------------------------
int TEqArtdaq::PrintProcesses(std::ostream& Stream) {
  int rc;
  TLOG(TLVL_DEBUG) << "-- START";
  TLOG(TLVL_DEBUG) << std::format("-- END rc:{}",rc);
  return rc;
}

//-----------------------------------------------------------------------------
// an equipment item can process commands sent to it only sequentially
// however different items can run in parallel
// also, can run command processing as a detached thread 
//-----------------------------------------------------------------------------
void TEqArtdaq::ProcessCommand(int hDB, int hKey, void* Info) {
  TLOG(TLVL_DEBUG) << "-- START";
  // in the end, ProcessCommand should send ss.str() as a message to some log
  std::stringstream ss;

  OdbInterface* odb_i = OdbInterface::Instance();
//-----------------------------------------------------------------------------
// based on the key, figure out own name and the node name
// - this is the price paid for decoupling
//-----------------------------------------------------------------------------
  KEY k;
  odb_i->GetKey(hKey,&k);

  HNDLE h_cmd = odb_i->GetParent(hKey);
//-----------------------------------------------------------------------------
// the command tree is assumed to have a form of .../mu2edaq09/DTC1/'
// so the frontend name is the same as the host label
//-----------------------------------------------------------------------------
  HNDLE h_frontend = odb_i->GetParent(h_cmd);
  KEY frontend;
  odb_i->GetKey(h_frontend,&frontend);
  
  TLOG(TLVL_DEBUG) << "k.name:" << k.name;

  //  std::string cmd_buf_path = std::format("/Mu2e/Commands/Frontends/{}/{}",frontend.name,dtc.name);

                                        // should be 0 or 1
  int run = odb_i->GetInteger(h_cmd,"Run");
  if (run == 0) {
    TLOG(TLVL_DEBUG) << "self inflicted, return";
    return;
  }
//-----------------------------------------------------------------------------
// get DTC config handle and set the DTC busy status
//-----------------------------------------------------------------------------
  // HNDLE h_dtc = odb_i->GetDtcConfigHandle(frontend.name,pcie_addr);
  // odb_i->SetInteger(h_dtc,"Status",1);
  
  std::string cmd            = odb_i->GetString (h_cmd,"Name");
  std::string parameter_path = odb_i->GetString (h_cmd,"ParameterPath");
  //  int link                   = odb_i->GetInteger(h_cmd,"link");
//-----------------------------------------------------------------------------
// this is address of the parameter record
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "cmd:" << cmd << " parameter_path:" << parameter_path;

  //  HNDLE h_par_path           = odb_i->GetHandle(0,parameter_path);
//-----------------------------------------------------------------------------
// should be already defined at this point
//-----------------------------------------------------------------------------
  TEqArtdaq*  eq = (TEqArtdaq*) TEquipmentManager::Instance()->_eq_artdaq;

  ss << std::format("host_label:{} host_name:{} cmd:{}",eq->HostLabel(),eq->FullHostName(),cmd);
//-----------------------------------------------------------------------------
// PRINT_PROCESSES
//------------------------------------------------------------------------------
  int cmd_rc(0);
  if (cmd == "print_processes") {
    cmd_rc = eq->PrintProcesses(ss);
  }
  else {
    ss << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }
//-----------------------------------------------------------------------------
// write output to the equipment log - need to revert the line order 
//-----------------------------------------------------------------------------
  cmd_rc = eq->WriteOutput(ss.str());
  
//-----------------------------------------------------------------------------
// done, avoid second call - leave "Run" = 1;, before setting it to 1 again,
// need to make sure that "Finished" = 1
//-----------------------------------------------------------------------------
  odb_i->SetInteger(h_cmd,"Finished",1);
//-----------------------------------------------------------------------------
// and set the DTC status
//-----------------------------------------------------------------------------
//  odb_i->SetInteger(h_dtc,"Status",cmd_rc);
  TLOG(TLVL_DEBUG) << "-- END";
}
