/////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "odbxx.h"

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

using namespace std;
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
  HNDLE h_artdaq_conf = _odb_i->GetHostArtdaqConfHandle(_h_active_run_conf,_host_label);
  HNDLE h_component;
  KEY   component;
  for (int i=0; db_enum_key(hDB, h_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
//-----------------------------------------------------------------------------
// use the component label 
// component names:
//                   brxx - board readers
//                   ebxx - event builders
//                   dlxx - data loggers
//                   dsxx - dispatchers
//-----------------------------------------------------------------------------
    db_get_key(hDB, h_component, &component);
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
  const std::string node_path     = "/Equipment/"+TMFeEquipment::fEqName;
  const std::string settings_path = node_path+"/Settings";
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
  
  // two words per process - N(segments/sec) and the data rate, MB/sec
  int            rc(0);
  xmlrpc_env     env;
  xmlrpc_value*  resultP;
  xmlrpc_client* clientP;
  
  BrMetrics_t    brm;
  
  const char* url = Ac->xmlrpc_url.data();
  TLOG(TLVL_DEBUG+1) << "000: Url:" << url;

  std::string res;
  int         nf(-1);

  try {
    xmlrpc_env_init(&env);  // P.M. : use _env - it should be initialized - may be not ?

    xmlrpc_client_create(&env, XMLRPC_CLIENT_NO_FLAGS, "read_br_metrics", "0.1", NULL,0,&clientP);
    int timeout_ms(1000);
    xmlrpc_client_event_loop_finish_timeout(clientP, timeout_ms);

                               // "({s:i,s:i})",
    xmlrpc_client_call2f(&env,clientP,url,"daq.report",&resultP, "(s)","stats");
    if (env.fault_occurred) {
      rc = env.fault_code;
      TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " output:" << env.fault_string;
      // throw;
      goto DONE_PARSING;
    }
//-----------------------------------------------------------------------------
// XML-RPC call successful, see what came back
//-----------------------------------------------------------------------------
    const char* value;
    size_t      length;
    xmlrpc_read_string_lp(&env, resultP, &length, &value);
    
    res = value;
    xmlrpc_DECREF   (resultP);
//-----------------------------------------------------------------------------
// parsing
//-----------------------------------------------------------------------------
    if (res.find("Failed") == 0) {
                                        // RPC comm failed
      rc = -1;
      TLOG(TLVL_ERROR) << "002 ERROR: res=" << res;
      // throw;
      goto DONE_PARSING;
    }
//-----------------------------------------------------------------------------
// parsing, normal output should contain 6 lines
//-----------------------------------------------------------------------------
    std::vector<std::string> lines;
    boost::split(lines,res,boost::is_any_of("\n"));
    int nlines = lines.size();
    TLOG(TLVL_DEBUG+1) << "001: nlines:" << nlines;
    if (nlines < 4+Ac->n_fragment_types) {
      rc = -10-nlines;
      TLOG(TLVL_ERROR) << "002 ERROR: nlines=" << nlines;
      // throw;
      goto DONE_PARSING;
    }
//-----------------------------------------------------------------------------
// first line
// 0:  br01 statistics:\n
//-----------------------------------------------------------------------------
    // boost::trim(lines[0]); 
    // TLOG(TLVL_DEBUG+1) << "002: lines[0]:" << lines[0];

    vector<string> words;                                // words of the lines[0]
    // boost::split(words,lines[0],boost::is_any_of(" ,"));

    // TLOG(TLVL_DEBUG+1) << "003: lines[0].words[4]:" << words[4] << " lines[0].words[10]:" << words[10];
    // brs.runNumber = std::stoi(words[ 4]);
    // brs.nFragTot  = std::stoi(words[10]);
//-----------------------------------------------------------------------------
// second line: 
// 1:    Fragments read: 74828 fragments generated at 623.496 getNext calls/sec, fragment rate = 1246.99 fragments/sec, monitor window = 60.0068 sec, min::max read size = 2::2 fragments  Average times per fragment:  elapsed time = 0.00160386 sec\n
//-----------------------------------------------------------------------------
    boost::trim(lines[1]); 

    words.clear();
    boost::split(words,lines[1],boost::is_any_of(" "));

    TLOG(TLVL_DEBUG+1) << "004: lines[1]:" << lines[1];
    TLOG(TLVL_DEBUG+1) << "005: lines[1].nwords:" << words.size();

    brm.nf_read      = std::stoi(words[ 2]);
    brm.getNextRate  = std::stof(words[ 6]);
    brm.fr_rate      = std::stof(words[12]);
    brm.time_window  = std::stof(words[17]);
    
    vector<string> n1;
    boost::split(n1,words[23],boost::is_any_of(":"));

    TLOG(TLVL_DEBUG+1) << "006: lines[1],words[23]:" << words[23] << " n1.size():" << n1.size() << " n1[0]:" << n1[0] << " n1[2]:" << n1[2] ;
    
    brm.min_nf      = std::stoi(n1[ 0]);
    brm.max_nf      = std::stoi(n1[ 2]);
    TLOG(TLVL_DEBUG+1) << "007: brm.min_nf: " << brm.min_nf << " brm.max_nf: " << brm.max_nf << " words[32]: " << words[32];
//-----------------------------------------------------------------------------
// it looks that there are some TAB characters hidden
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG+1) << "008: words[32]: " << words[32] << " words[33]: " << words[33];
    brm.elapsed_time = std::stof(words[33]);
    TLOG(TLVL_DEBUG+1) << "009: brm.elapsed_time: " << brm.elapsed_time;
//-----------------------------------------------------------------------------
// third line
// Fragment output statistics: 74827 fragments sent at 1246.97 fragments/sec, effective data rate = 0.584226 MB/sec, monitor window = 60.0068 sec, min::max event size = 0.000457764::0.000534058 MB\n
//-----------------------------------------------------------------------------
    boost::trim(lines[2]); 
    TLOG(TLVL_DEBUG+1) << "010: lines[2]:" << lines[2];

    words.clear();
    boost::split(words,lines[2],boost::is_any_of(" "));

    brm.nf_sent     = std::stof(words[ 3]);
    brm.output_fr   = std::stof(words[ 7]);
    brm.output_dr   = std::stof(words[13]);
    brm.output_mw   = std::stof(words[18]);

    vector<string> n2;
    boost::split(n2,words[24],boost::is_any_of(":"));
    brm.min_evt_size = std::stof(n2[ 0]);
    brm.max_evt_size = std::stof(n2[ 2]);

    TLOG(TLVL_DEBUG+1) << "011: lines[2].nwords:" << lines[2].size()
                       << " words[3]:" << words[3] << " words[7]:" << words[7]
                       << " n2[0]:" << n2[0] << " n2[2]:" << n2[2];
//-----------------------------------------------------------------------------
// line # 4
//    Input wait time = 0.00156023 s/fragment, buffer wait time = 4.04943e-05 s/fragment, request wait time = 0.00075403 s/fragment, output wait time = 3.5861e-05 s/fragment\n
//-----------------------------------------------------------------------------
    boost::trim(lines[3]); 

    words.clear();
    boost::split(words,lines[3],boost::is_any_of(" "));
    TLOG(TLVL_DEBUG+1) << "012: words[4]:" << words[4] << " words[10]:" << words[10] 
                         << " words[16]:" << words[16]  << " words[22]:" << words[22];
    brm.inp_wait_time = std::stof(words[ 4]);
    brm.buf_wait_time = std::stof(words[10]);
    brm.req_wait_time = std::stof(words[16]);
    brm.out_wait_time = std::stof(words[22]);
//-----------------------------------------------------------------------------
// n fragments
// 4: fragment_id:0 nfragments:0 nbytes:0 max_nf:1000 max_nb:1048576000
//-----------------------------------------------------------------------------
    nf = nlines-4;
    if (nf > 5) nf = 5;

    TLOG(TLVL_DEBUG+1) << "013: nf = " << nf;

    for (int i=0; i<nf; i++) {
      boost::trim(lines[4+i]); 

      TLOG(TLVL_DEBUG+1) << "014: i=" << i << " lines[4+i]:" << lines[4+i];
      words.clear();
      boost::split(words,lines[4+i],boost::is_any_of(" "));
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
      TLOG(TLVL_DEBUG+1) << "015: words.size()" << words.size();
      int nww = words.size();
      for (int k=0; k<nww; k++) TLOG(TLVL_DEBUG+1) << "016: k:" << k << "words[k]:" << words[k];

      try {
        vector<string> num;
        boost::split(num,words[0],boost::is_any_of(":"));
        brm.fr_id [i] = std::stoi (num[ 1]);
      
        num.clear();
        boost::split(num,words[1],boost::is_any_of(":"));
        brm.nf_shm[i] = std::stoi (num[ 1]);

        num.clear();
        boost::split(num,words[2],boost::is_any_of(":"));
        brm.nb_shm[i] = std::stoi(num[ 1]);
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "coudnt parse:" << lines[4+i];
      }
    }
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "res:" << res;
  }
//-----------------------------------------------------------------------------
// done with parsing, copy to the Data
//-----------------------------------------------------------------------------
 DONE_PARSING:
  char buf[1024];
  
  ComposeEvent(buf, sizeof(buf));
  BkInit      (buf, sizeof(buf));
  double* ptr = (double*) BkOpen(buf, Ac->name.data(), TID_DOUBLE);
     
  *ptr++ = brm.nf_read;        // 
  *ptr++ = brm.getNextRate;
  *ptr++ = brm.fr_rate;
  *ptr++ = brm.time_window;
  *ptr++ = brm.min_nf;
  *ptr++ = brm.max_nf;
  *ptr++ = brm.elapsed_time;
  *ptr++ = brm.nf_sent;
  *ptr++ = brm.output_fr;
  *ptr++ = brm.output_dr;
  *ptr++ = brm.output_mw;
  *ptr++ = brm.min_evt_size;
  *ptr++ = brm.max_evt_size;
  *ptr++ = brm.inp_wait_time;
  *ptr++ = brm.buf_wait_time;
  *ptr++ = brm.req_wait_time;
  *ptr++ = brm.out_wait_time;
//-----------------------------------------------------------------------------
// shared memory
//-----------------------------------------------------------------------------
  if (nf <= 5) {
    for (int i=0; i<nf; i++) {
      *ptr++ = brm.fr_id [i];
      *ptr++ = brm.nf_shm[i];
      *ptr++ = brm.nb_shm[i];
    }
  }
  else {
    TLOG(TLVL_ERROR) << "010: nf=" << nf;
  }
  
  BkClose    (buf,ptr);
  EqSendEvent(buf);
//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
//  Data[30] = rc;

  TLOG(TLVL_DEBUG+1) << "rc=" << rc;
 
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
int TEquipmentNode::ReadDataReceiverMetrics(const ArtdaqComponent_t* Ac) {
  
  // two words per process - N(segments/sec) and the data rate, MB/sec
  int            rc(0);
  xmlrpc_env     env;
  xmlrpc_value*  resultP;
  xmlrpc_client* clientP;

  EbMetrics_t    drm;
  
  const char* url = Ac->xmlrpc_url.data();
  TLOG(TLVL_DEBUG+1) << "000: Url:" << url;

  std::string res;

  try {
    xmlrpc_env_init(&env);
    xmlrpc_client_create(&env, XMLRPC_CLIENT_NO_FLAGS, "read_dr_metrics", "0.1", NULL,0,&clientP);
    int timeout_ms(1000);
    xmlrpc_client_event_loop_finish_timeout(clientP, timeout_ms);

    xmlrpc_client_call2f(&env,clientP,url,"daq.report",&resultP, "(s)","stats");
    if (env.fault_occurred) {
      TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string;
      rc = env.fault_code;
      goto DONE_PARSING_1;
    }

    const char* value;
    size_t      length;
    xmlrpc_read_string_lp(&env, resultP, &length, &value);
    
    res = value;
    xmlrpc_DECREF   (resultP);

    TLOG(TLVL_DEBUG+1) << "001: res:" << res;
    if (res.find("Failed") == 0) {
                                        // RPC comm failed
      rc = -1;
      TLOG(TLVL_ERROR) << "001 ERROR: res:" << res;
      // throw;
      goto DONE_PARSING_1;
    }
//-----------------------------------------------------------------------------
//  now the output string needs to be parsed. it should look as follows
//-----------------------------------------------------------------------------
    vector<string> lines;
    boost::split(lines,res,boost::is_any_of("\n"));
//-----------------------------------------------------------------------------
// normally, the data receiver response has 6 lines in it.
// however, sometimes get back only one line saying "busy": "001: res:busy"
// handle that:
//-----------------------------------------------------------------------------
    int nlines = lines.size();
    if (nlines < 6) {
      rc = -10-nlines;
//-----------------------------------------------------------------------------
// what do I do wrong with throwing an exception ?
//-----------------------------------------------------------------------------
      // throw;
      TLOG(TLVL_ERROR) << "002 ERROR: nlines:" << nlines;
      goto DONE_PARSING_1;
    }
//-----------------------------------------------------------------------------
// skip line #0. Proceed with line #1:
//    Event statistics: 20200 events released at 336.642 events/sec, effective data rate = 6.14159 MB/sec, monitor window = 60.0045 sec, min::max event size = 0.0150223::0.0223465 MB
//-----------------------------------------------------------------------------
    boost::trim(lines[1]);
    TLOG(TLVL_DEBUG+1) << "002: lines[1]:" << lines[1];

    vector<string> words;                                // words of the lines[0]
    boost::split(words,lines[1],boost::is_any_of(" "));
    drm.nev_read    = std::stoi(words[ 2]);
    drm.evt_rate    = std::stof(words[ 6]);
    drm.data_rate   = std::stof(words[12]);
    drm.time_window = std::stof(words[17]);

    vector<string> num;
    boost::split(num,words[23],boost::is_any_of(":"));

    drm.min_evt_size = std::stof(num[ 0]);
    drm.max_evt_size = std::stof(num[ 2]);
//-----------------------------------------------------------------------------
// line #2:
//    Average time per event:  elapsed time = 0.00281596 sec\n
//-----------------------------------------------------------------------------
    boost::trim(lines[2]);
    TLOG(TLVL_DEBUG+1) << "003: lines[2]:" << lines[2];

    words.clear();                      // words of the lines[2]
    boost::split(words,lines[2],boost::is_any_of(" "));
    TLOG(TLVL_DEBUG+1) << "004: lines[2].words[8]:" << words[8];
    drm.etime  = std::stof(words[ 8]);
//-----------------------------------------------------------------------------
// line #3:
//  Fragment statistics: 20000 fragments received at 333.309 fragments/sec, effective data rate = 3.87958 MB/sec, monitor window = 60.0043 sec, min::max fragment size = 0.00602722::0.0196686 MB
//-----------------------------------------------------------------------------
    boost::trim(lines[3]);
    TLOG(TLVL_DEBUG+1) << "005: lines[3]:" << lines[3];

    words.clear();                      // words of the lines[3]
    boost::split(words,lines[3],boost::is_any_of(" "));

    drm.nfr_read     = std::stoi(words[ 2]);
    drm.nfr_rate     = std::stof(words[ 6]);
    drm.fr_data_rate = std::stof(words[12]);

    num.clear();
    boost::split(num,words[23],boost::is_any_of(":"));
    TLOG(TLVL_DEBUG+1) << "006: lines[3].words[23]:" << words[23] << " num[0]:" << num[0] << " num[2]:" << num[2];
//-----------------------------------------------------------------------------
// sometimes the report looks like:
// Event statistics: 0 events released at 0 events/sec, effective data rate = 0 MB/sec, monitor window = 0 sec, min::max event size = inf::-inf
//-----------------------------------------------------------------------------
    if (num[0] == "inf") {
      rc = -3;
      TLOG(TLVL_ERROR) << "006: num[0]=" << num[0];
      //      throw;
      goto DONE_PARSING_1;
    }

    drm.min_fr_size = std::stof(num[ 0]);
    drm.max_fr_size = std::stof(num[ 2]);
//-----------------------------------------------------------------------------
// line #4:
//    Event counts: Run -- 1200271 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. \n
//-----------------------------------------------------------------------------
    boost::trim(lines[4]);
    TLOG(TLVL_DEBUG+1) << "lines[4]:" << lines[4];

    words.clear();                      // words of the lines[4]
    boost::split(words,lines[4],boost::is_any_of(" "));

    TLOG(TLVL_DEBUG+1) << "lines[4].words[4]:" << words[4] << " words[6]:" << words[6] 
                         << " words[11]:" << words[11] << " words[13]:" << words[13] ;

    drm.nev_tot_rn = std::stoi(words[ 4]);
    drm.nev_bad_rn = std::stoi(words[ 6]);
    drm.nev_tot_sr = std::stoi(words[11]);
    drm.nev_bad_sr = std::stoi(words[13]);
//-----------------------------------------------------------------------------
// line #5:
// shm_nbb :10:1048576:0:0:10:0\n
//-----------------------------------------------------------------------------
    boost::trim(lines[5]);
    TLOG(TLVL_DEBUG+1) << "lines[5]:" << lines[5];

    words.clear();                      // words of the lines[5]
    boost::split(words,lines[5],boost::is_any_of(":"));

    drm.nbuf_shm_tot   = std::stoi(words[ 1]);
    drm.nbytes_shm_tot = std::stoi(words[ 2]);
    drm.nbuf_shm_empty = std::stoi(words[ 3]);
    drm.nbuf_shm_write = std::stoi(words[ 3]);
    drm.nbuf_shm_full  = std::stoi(words[ 4]);
    drm.nbuf_shm_read  = std::stoi(words[ 5]);
  }
  catch (...) {
//-----------------------------------------------------------------------------
// any kind of error 
//-----------------------------------------------------------------------------
    TLOG(TLVL_ERROR) << "url:" << url << " response:" << res;
  }
//-----------------------------------------------------------------------------
// copy data to the output
//-----------------------------------------------------------------------------
 DONE_PARSING_1:

  char buf[1024];
  
  ComposeEvent(buf, sizeof(buf));
  BkInit      (buf, sizeof(buf));
  float* ptr = (float*) BkOpen(buf, Ac->name.data(), TID_FLOAT);
     
  *ptr++ = drm.nev_read;        // 
  *ptr++ = drm.evt_rate;
  *ptr++ = drm.data_rate;
  *ptr++ = drm.time_window;
  *ptr++ = drm.min_evt_size;
  *ptr++ = drm.max_evt_size;
  *ptr++ = drm.etime;

  *ptr++ = drm.nfr_read;
  *ptr++ = drm.nfr_rate;
  *ptr++ = drm.fr_data_rate;
  *ptr++ = drm.min_fr_size;
  *ptr++ = drm.max_fr_size;
  
  *ptr++ = drm.nev_tot_rn;
  *ptr++ = drm.nev_bad_rn;
  *ptr++ = drm.nev_tot_sr;
  *ptr++ = drm.nev_bad_sr;

  *ptr++ = drm.nbuf_shm_tot;
  *ptr++ = drm.nbytes_shm_tot;
  *ptr++ = drm.nbuf_shm_empty;
  *ptr++ = drm.nbuf_shm_write;
  *ptr++ = drm.nbuf_shm_full;
  *ptr++ = drm.nbuf_shm_read;
  
  BkClose    (buf,ptr);
  EqSendEvent(buf);

  return rc;
}
  
//-----------------------------------------------------------------------------
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
    else if (ac->type == kEventBuilder) ReadDataReceiverMetrics(ac);
    else if (ac->type == kDataLogger  ) ReadDataReceiverMetrics(ac);
    else if (ac->type == kDispatcher  ) ReadDsMetrics(ac);
  }

}
