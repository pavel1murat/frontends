/////////////////////////////////////////////////////////////////////////////
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

using namespace std;
using nlohmann::json;

#include <boost/algorithm/string.hpp>

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode_Artdaq"

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

//-----------------------------------------------------------------------------
// init ODB structure no matter what
TMFeResult TEquipmentNode::InitArtdaq() {
//-----------------------------------------------------------------------------
// get port number used by the TFM, don't assume the farm_manager is running locally
// the frontend has to have its own xmlrpc URL,
// TFM uses port           10000+1000*partition
// boardreaders start from 10000+1000*partition+100+1;
// init XML RPC            10000+1000*partition+11
//-----------------------------------------------------------------------------
  // int         partition   = _odb_i->GetArtdaqPartition(hDB);
  // int         port_number = 10000+1000*partition+11;
  
  char cbuf[100];

  TLOG(TLVL_DEBUG) << "START";

  sprintf(cbuf,"%s_mon",_full_host_name.data());
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS,cbuf,"v1_0");
  xmlrpc_env_init(&_env);
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
  
  InitArtdaqVarNames();
  
  TLOG(TLVL_DEBUG) << "END";
  return TMFeOk();
}

//-----------------------------------------------------------------------------
// boardreaders: 'br01', 'br02' , etc
//-----------------------------------------------------------------------------
void TEquipmentNode::InitArtdaqVarNames() {
  char dirname[128], name[128];

  TLOG(TLVL_DEBUG) << "START";
  const std::string node_odb_path = "/Equipment/"+TMFeEquipment::fEqName;
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
}

//-----------------------------------------------------------------------------
int TEquipmentNode::ReadBrMetrics(const ArtdaqComponent_t* Ac) {
  int rc(0);
  TLOG(TLVL_DEBUG+1) << "-- START";

  std::regex pattern(R"(/RPC2$)");
  std::string url = std::regex_replace(Ac->xmlrpc_url, pattern, "");

  std::string cmd = std::format("python config/scripts/artdaq_xmlrpc.py --test=parse_br_metrics --url={}",url);

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  json brm = json::parse(output);

  TLOG(TLVL_DEBUG+1) << "parsed json:" << brm << std::endl;

  char buf[1024];
  
  ComposeEvent(buf, sizeof(buf));
  BkInit      (buf, sizeof(buf));
  double* ptr = (double*) BkOpen(buf, Ac->name.data(), TID_DOUBLE);
     
  *ptr++ = brm["nf_read"];        // 
  *ptr++ = brm["gn_rate"];
  *ptr++ = brm["fr_rate"];
  *ptr++ = brm["time_win"];
  *ptr++ = brm["min_nf"];
  *ptr++ = brm["max_nf"];
  *ptr++ = brm["elapsed_time"];
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
    *ptr      = brm["fids"][i]["fid"   ];
    *(ptr+5)  = brm["fids"][i]["max_nf"];
    *(ptr+10) = brm["fids"][i]["max_nb"];
    ptr++;
  }
  TLOG(TLVL_DEBUG+1) << "repacking 2";
//-----------------------------------------------------------------------------
// record has a fixed length !
//-----------------------------------------------------------------------------
  ptr = psave+15;
  BkClose    (buf,ptr);
  EqSendEvent(buf);
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
int TEquipmentNode::ReadDrMetrics(const ArtdaqComponent_t* Ac) {
  int rc(0);
  
  TLOG(TLVL_DEBUG+1) << "-- START";

  std::regex pattern(R"(/RPC2$)");
  std::string url = std::regex_replace(Ac->xmlrpc_url, pattern, "");

  std::string cmd = std::format("python config/scripts/artdaq_xmlrpc.py --test=parse_dr_metrics --url={}",url);

  TLOG(TLVL_DEBUG+1) << "cmd=" << cmd;
  
  std::string output  = popen_shell_command(cmd);

  TLOG(TLVL_DEBUG+1) << "output=" << output;

  json drm = json::parse(output);

  TLOG(TLVL_DEBUG+1) << "parsed json:" << drm;
//-----------------------------------------------------------------------------
// copy data to the output
//-----------------------------------------------------------------------------
  char buf[1024];
  
  ComposeEvent(buf, sizeof(buf));
  BkInit      (buf, sizeof(buf));
  float* ptr = (float*) BkOpen(buf, Ac->name.data(), TID_FLOAT);
     
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

  BkClose    (buf,ptr);
  EqSendEvent(buf);

  TLOG(TLVL_DEBUG+1) << "-- END, rc:" << rc;

  return rc;
}

  
//-----------------------------------------------------------------------------
int TEquipmentNode::ReadDsMetrics(const ArtdaqComponent_t* Ac) {
  return 0;
}
  
//-----------------------------------------------------------------------------
// this is happening within the node
// EB and DL have the same metrics
//-----------------------------------------------------------------------------
void TEquipmentNode::ReadArtdaqMetrics() {
  int nbr = _list_of_ac.size();
  for (int i=0; i<nbr; i++) {
    ArtdaqComponent_t* ac = &_list_of_ac[i];
    if      (ac->type == kBoardReader ) ReadBrMetrics(ac);
    else if (ac->type == kEventBuilder) ReadDrMetrics(ac);
    else if (ac->type == kDataLogger  ) ReadDrMetrics(ac);
    else if (ac->type == kDispatcher  ) ReadDsMetrics(ac);
  }

}
