///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// TFM slow control driver
// for now it stores 7 parameters:
// -------------------------------
// 0: boardreader : N (getNext calls)/sec 
// 1: boardreader : N fragments/sec
// 2: boardreader : data rate (MBytes/sec)
// 3: eventbuilder: N events/sec
// 4: eventbuilder: data rate (MBytes/sec)
// 5: datalogger  : N events/sec
// 6: datalogger  : data rate (MBytes/sec)
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define TRACE_NAME "tfm_driver"

#include <time.h>  
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>

#include "midas.h"

#include <vector>
#include <boost/algorithm/string.hpp>

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tfm_frontend/tfm_driver.hh"

using std::vector, std::string;

// static strings _fn("tfm_driver.log");
// static FILE*  _f;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_DRIVER_SETTINGS_STR "\
link   = INT : 0\n\
active = INT : 0\n\
"

typedef INT(func_t) (INT cmd, ...);

static char         _artdaq_conf[50];
static int          _partition;
static int          _base_port_number;

static double       _prev_time_sec (-1);
static double       _prev_sz_mbytes(-1);

static std::string  _brXmlrpcUrl;       // XML-RPC url of the first board reader
static std::string  _ebXmlrpcUrl;       // XML-RPC url of the first event builder
static std::string  _dlXmlrpcUrl;       // XML-RPC url of the first data logger
//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqNodeKey, const char* ComponentName, std::string& Url) {

  char  hostname[50];
  int   sz = sizeof(hostname);
  db_get_value(hDB,hArtdaqNodeKey,"Hostname", hostname, &sz, TID_STRING, TRUE);
  TLOG(TLVL_INFO+10) << "001 hostname:" << hostname << " ComponentName:" << ComponentName;

  HNDLE hComponentKey; 

  try {
    db_find_key(hDB,hArtdaqNodeKey,ComponentName,&hComponentKey);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "001.1: failed to get the key";
  }
  

  TLOG(TLVL_INFO+10) << "001.2 hostname:" << hostname << " ComponentName:" << ComponentName;
//-----------------------------------------------------------------------------
// if port is defined explicitly, use that
// otherwise, the port number is defined by the executable rank 
//-----------------------------------------------------------------------------
  int   port_number;
  HNDLE hPortKey; 
  sz = sizeof(int);
  if (db_find_key(hDB, hComponentKey, "XmlrpcPort", &hPortKey) == DB_SUCCESS) {
    db_get_value(hDB, hComponentKey, "XmlrpcPort", &port_number, &sz, TID_INT32, FALSE);
  }
  else {
    int rank;
    db_get_value(hDB, hComponentKey, "Rank", &rank, &sz, TID_INT32, FALSE);
    port_number = _base_port_number+100+rank;
  }

  TLOG(TLVL_INFO+10) << "001.3 port_number:" << port_number;

  std::string local_hostname;
  char buf[100];
  FILE* pipe = popen("hostname -f", "r");
  while (!feof(pipe)) {
    char* s = fgets(buf, 100, pipe);
    if (s) local_hostname += buf;
  }
  pclose(pipe);

  TLOG(TLVL_INFO+10) << "002.1 local_hostname:" << local_hostname;
  
  if (strcmp(hostname,local_hostname.data()) == 0) strcpy(hostname,"localhost");

  Url = "http://"+std::string(hostname)+":"+std::to_string(port_number)+"/RPC2";

  TLOG(TLVL_INFO+10) << "003 _brUrl:" << Url;
  return 0;
}

//-----------------------------------------------------------------------------
// device driver routines
// the init function creates a ODB record which contains the
// settings and initialized it variables as well as the bus driver
//-----------------------------------------------------------------------------
INT tfm_driver_init(HNDLE hkey, TFM_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int             status, size;
  HNDLE           hDB   , hkeydd;
  TFM_DRIVER_INFO *info;
//-----------------------------------------------------------------------------
// allocate info structure
//-----------------------------------------------------------------------------
  info = (TFM_DRIVER_INFO*) calloc(1, sizeof(TFM_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

  char        active_conf[100];
  int   sz = sizeof(active_conf);
  db_get_value(hDB, 0, "/Experiment/ActiveConfiguration", &active_conf, &sz, TID_STRING, TRUE);

  HNDLE      hActiveConfKey;
  char       key[200];
  sprintf(key,"/Experiment/RunConfigurations/%s",active_conf);
	db_find_key(hDB, 0, key, &hActiveConfKey);

  sz = sizeof(int);
  db_get_value(hDB, hActiveConfKey, "ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
  _base_port_number = 10000+1000*_partition;
//-----------------------------------------------------------------------------
// find out the port numbers for the first boardreader, first event builder, 
// and first datalogger
//-----------------------------------------------------------------------------
  sz = sizeof(_artdaq_conf);
  db_get_value(hDB,hActiveConfKey,"ArtdaqConfiguration", &_artdaq_conf, &sz, TID_STRING, TRUE);
  TLOG(TLVL_INFO+10) << "001 artdaq_conf:" << _artdaq_conf;

  sprintf(key,"/ArtdaqConfigurations/%s",_artdaq_conf);
  std::string k1 = key;

  HNDLE h_active_conf;
	if (db_find_key(hDB, 0, k1.data(), &h_active_conf) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "0012 no handle for:" << k1 << ", got:" << h_active_conf;
  }

  HNDLE hArtdaqNodeKey;
  sprintf(key,"/ArtdaqConfigurations/%s/Node_01",_artdaq_conf);
  k1 = key;

	if (db_find_key(hDB, 0, k1.data(), &hArtdaqNodeKey) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no handle for:" << key << ", got" << hArtdaqNodeKey;
  }

  get_xmlrpc_url(hDB,hArtdaqNodeKey,"BoardReader_01" ,_brXmlrpcUrl);
  get_xmlrpc_url(hDB,hArtdaqNodeKey,"EventBuilder_01",_ebXmlrpcUrl);
  get_xmlrpc_url(hDB,hArtdaqNodeKey,"DataLogger_01"  ,_dlXmlrpcUrl);
//-----------------------------------------------------------------------------
// create DRIVER settings record 
//-----------------------------------------------------------------------------
  status = db_create_record(hDB, hkey, "DD", TFM_DRIVER_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  db_find_key(hDB, hkey, "DD", &hkeydd);
  size = sizeof(info->driver_settings);
  db_get_record(hDB, hkeydd, &info->driver_settings, &size, 0);
//------------------------------------------------------------------------------
// initialize driver 
// info->array seems to be an internal array, use it to read
//-----------------------------------------------------------------------------
  info->num_channels = channels;
  info->array = (float*) calloc(channels, sizeof(float));
  info->bd = bd;
  info->hkey = hkey;

  if (!bd)  return FE_ERR_ODB;

  /* initialize bus driver */
  status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
  
  if (status != SUCCESS)
    return status;
  
  /* initialization of device, something like ... */
  BD_PUTS("init");

  //  _f = fopen(_fn.data(),"w");
  
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT tfm_driver_exit(TFM_DRIVER_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);

  //   delete _dtc;

  //  fclose(_f);
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT tfm_driver_set(TFM_DRIVER_INFO * info, INT channel, float value) {
  char str[80];

// set channel to a specific value, something like ...
  sprintf(str, "SET %d %lf", channel, value);
  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);

// simulate writing by storing value in local array, has to be removed in a real driver
  if (channel < info->num_channels)
    info->array[channel] = value;

  return FE_SUCCESS;
}


//-----------------------------------------------------------------------------
// the BR stat report string needs to be parsed. The report looks as follows:
// ----------------------------------------------------------------------------
// 0:  boardreader01 run number = 105251, Sent Fragment count = 5484253, boardreader01 statistics:\n
// 1:    Fragments read: 74828 fragments generated at 623.496 getNext calls/sec, fragment rate = 1246.99 fragments/sec, monitor window = 60.0068 sec, min::max read size = 2::2 fragments  Average times per fragment:  elapsed time = 0.00160386 sec\n
// 2:    Fragment output statistics: 74827 fragments sent at 1246.97 fragments/sec, effective data rate = 0.584226 MB/sec, monitor window = 60.0068 sec, min::max event size = 0.000457764::0.000534058 MB\n
// 3:    Input wait time = 0.00156023 s/fragment, buffer wait time = 4.04943e-05 s/fragment, request wait time = 0.00075403 s/fragment, output wait time = 3.5861e-05 s/fragment\n
// 4: fragment_id: 11 nfragments: 0 nbytes: 0 max_nf: 1000 max_nb: 1048576000\n
// 5: fragment_id: 0 nfragments: 0 nbytes: 0 max_nf: 1000 max_nb: 1048576000\n
//-----------------------------------------------------------------------------
int get_boardreader_data(const char* Url, float* Data) {
  // two words per process - N(segments/sec) and the data rate, MB/sec
  int           rc(0);
  xmlrpc_env    env;
  xmlrpc_value* resultP;

  struct BrStatData_t {
    int    runNumber;
    int    nFragTot;                    // end of line 0
    int    nFragRead;
    float  getNextRate;                 // per second;
    float  fragRate;
    float  timeWindow;
    int    minNFrag;                    // per getNext call
    int    maxNFrag;                    // per getNext call
    float  elapsedTime; 
    float  dataRate;                    // MB/sec
    float  minEventSize;                // MB
    float  maxEventSize;                // MB , end of line 1
    float  inputWaitTime;               // 
    float  bufferWaitTime;
    float  requestWaitTime;
    float  outputWaitTime;              // per fragment; end of line 2
    int    fragID  [5];                 // fragment ID's
    int    nFragsID[5];                 // n(fragments) currently in the buffer
    int    nBytesID[5];                 // n(bytes) currently in the buffer
  } brs;

  TLOG(TLVL_INFO + 10) << "000: Url:" << Url;
  memset(&brs,0,sizeof(BrStatData_t));

  std::string   res;
  int           nf(-1);
  try {
    xmlrpc_env_init(&env);
                               // "({s:i,s:i})",
    resultP = xmlrpc_client_call(&env,Url,"daq.report","(s)","stats");

    if (env.fault_occurred) {
      TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string;
      rc = env.fault_code;
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
      // throw;
      goto DONE_PARSING;
    }
//-----------------------------------------------------------------------------
// normal output should contain 6 lines, handle communication failures
//-----------------------------------------------------------------------------
    vector<string> lines;
    boost::split(lines,res,boost::is_any_of("\n"));

    int nlines = lines.size();
    TLOG(TLVL_INFO + 10) << "001: nlines:" << nlines;
    if (nlines < 6) {
      TLOG(TLVL_ERROR) << "ERROR: nlines:" << nlines;
      rc = -10-nlines;
      // throw;
      goto DONE_PARSING;
    }
//-----------------------------------------------------------------------------
// first line
// 0:  boardreader01 run number = 105251, Sent Fragment count = 5484253, boardreader01 statistics:\n
//-----------------------------------------------------------------------------
    boost::trim(lines[0]); 
    TLOG(TLVL_INFO + 10) << "002: lines[0]:" << lines[0];

    vector<string> words;                                // words of the lines[0]
    boost::split(words,lines[0],boost::is_any_of(" ,"));

    TLOG(TLVL_INFO + 10) << "003: lines[0].words[4]:" << words[4] << " lines[0].words[10]:" << words[10];
    brs.runNumber = std::stoi(words[ 4]);
    brs.nFragTot  = std::stoi(words[10]);
//-----------------------------------------------------------------------------
// second line: 
// 1:    Fragments read: 74828 fragments generated at 623.496 getNext calls/sec, fragment rate = 1246.99 fragments/sec, monitor window = 60.0068 sec, min::max read size = 2::2 fragments  Average times per fragment:  elapsed time = 0.00160386 sec\n
//-----------------------------------------------------------------------------
    boost::trim(lines[1]); 

    words.clear();
    boost::split(words,lines[1],boost::is_any_of(" "));

    TLOG(TLVL_INFO + 10) << "004: lines[1]:" << lines[1];
    TLOG(TLVL_INFO + 10) << "005: lines[1].nwords:" << words.size();

    brs.nFragRead    = std::stoi(words[ 2]);
    brs.getNextRate  = std::stof(words[ 6]);
    brs.fragRate     = std::stof(words[12]);
    brs.timeWindow   = std::stof(words[17]);
    
    vector<string> n1;
    boost::split(n1,words[23],boost::is_any_of(":"));

    TLOG(TLVL_INFO + 10) << "006: lines[1],words[23]:" << words[23] << " n1.size():" << n1.size() << " n1[0]:" << n1[0] << " n1[2]:" << n1[2] ;
    
    brs.minNFrag    = std::stoi(n1[ 0]);
    brs.maxNFrag    = std::stoi(n1[ 2]);
    TLOG(TLVL_INFO + 10) << "007: brs.minNFrag: " << brs.minNFrag << " brs.maxNFrag: " << brs.maxNFrag << " words[32]: " << words[32];
//-----------------------------------------------------------------------------
// it looks that there are some TAB characters hidden
//-----------------------------------------------------------------------------
    TLOG(TLVL_INFO + 10) << "008: words[32]: " << words[32] << " words[33]: " << words[33];
    brs.elapsedTime = std::stof(words[33]);
    TLOG(TLVL_INFO + 10) << "009: brs.elapsedTime: " << brs.elapsedTime;
//-----------------------------------------------------------------------------
// third line
// 2:    Fragment output statistics: 74827 fragments sent at 1246.97 fragments/sec, effective data rate = 0.584226 MB/sec, monitor window = 60.0068 sec, min::max event size = 0.000457764::0.000534058 MB\n
//-----------------------------------------------------------------------------
    boost::trim(lines[2]); 
    TLOG(TLVL_INFO + 10) << "010: lines[2]:" << lines[2];

    words.clear();
    boost::split(words,lines[2],boost::is_any_of(" "));

    brs.dataRate    = std::stof(words[13]);

    vector<string> n2;
    boost::split(n2,words[24],boost::is_any_of(":"));
    brs.minEventSize = std::stof(n2[ 0]);
    brs.maxEventSize = std::stof(n2[ 2]);

    TLOG(TLVL_INFO + 10) << "011: words[13]:" << words[13] << " words[24]:" << words[24] << " n2[0]:" << n2[0] << " n2[2]:" << n2[2];
//-----------------------------------------------------------------------------
// line # 4
//    Input wait time = 0.00156023 s/fragment, buffer wait time = 4.04943e-05 s/fragment, request wait time = 0.00075403 s/fragment, output wait time = 3.5861e-05 s/fragment\n
//-----------------------------------------------------------------------------
    boost::trim(lines[3]); 

    words.clear();
    boost::split(words,lines[3],boost::is_any_of(" "));
    TLOG(TLVL_INFO + 10) << "012: words[4]:" << words[4] << " words[10]:" << words[10] 
                         << " words[16]:" << words[16]  << " words[22]:" << words[22];
    brs.inputWaitTime   = std::stof(words[ 4]);
    brs.bufferWaitTime  = std::stof(words[10]);
    brs.requestWaitTime = std::stof(words[16]);
    brs.outputWaitTime  = std::stof(words[22]);
//-----------------------------------------------------------------------------
// n fragments
// 4: fragment_id: 11 nfragments: 0 nbytes: 0 max_nf: 1000 max_nb: 1048576000\n
//-----------------------------------------------------------------------------
    nf = nlines-4;
    if (nf > 5) nf = 5;

    TLOG(TLVL_INFO + 10) << "013: nf = " << nf;

    for (int i=0; i<nf; i++) {
      boost::trim(lines[4+i]); 

      TLOG(TLVL_INFO + 10) << "014: i=" << i << " lines[4+i]:" << lines[4+i];
      words.clear();
      boost::split(words,lines[4+i],boost::is_any_of(" "));
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
      TLOG(TLVL_INFO + 10) << "015: words.size()" << words.size();
      int nww = words.size();
      for (int k=0; k<nww; k++) TLOG(TLVL_INFO + 10) << "016: k:" << k << "words[k]:" << words[k];

      try {
        vector<string> num;
        boost::split(num,words[0],boost::is_any_of(":"));
        brs.fragID  [i] = std::stoi (num[ 1]);
      
        num.clear();
        boost::split(num,words[1],boost::is_any_of(":"));
        brs.nFragsID[i] = std::stoi (num[ 1]);

        num.clear();
        boost::split(num,words[2],boost::is_any_of(":"));
        brs.nBytesID[i] = std::stoi(num[ 1]);
      }
      catch (...) {
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
  Data[ 0] = brs.runNumber;        // N(getNext calls)
  Data[ 1] = brs.nFragTot ;        // N(fragments/sec)
  Data[ 2] = brs.nFragRead;        // data rate, MB/sec
  Data[ 3] = brs.getNextRate;
  Data[ 4] = brs.fragRate;
  Data[ 5] = brs.timeWindow;
  Data[ 6] = brs.minNFrag;
  Data[ 7] = brs.maxNFrag;
  Data[ 8] = brs.elapsedTime;
  Data[ 9] = brs.dataRate;
  Data[10] = brs.minEventSize;
  Data[11] = brs.maxEventSize;
  Data[12] = brs.inputWaitTime;
  Data[13] = brs.bufferWaitTime;
  Data[14] = brs.outputWaitTime;

  for (int i=0; i<nf; i++) {
    Data[15+3*i] = brs.fragID  [i];
    Data[16+3*i] = brs.nFragsID[i];
    Data[17+3*i] = brs.nBytesID[i];
  }
//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
  Data[30] = rc;

  TLOG(TLVL_INFO + 10) << "Data:" << Data[0] << " " << Data[1] << " rc=" << rc;
 
  return rc;
}

//-----------------------------------------------------------------------------
// parse EB and DL statistics report:
// ----------------------------------
// 0 datalogger01 statistics:\n
// 1   Event statistics: 21309 events released at 355.118 events/sec, effective data rate = 0.733651 MB/sec, monitor window = 60.0054 sec, min::max event size = 0.00205994::0.00212097 MB\n
// 2   Average time per event:  elapsed time = 0.00281596 sec\n
// 3   Fragment statistics: 21309 fragments received at 355.118 fragments/sec, effective data rate = 0.720105 MB/sec, monitor window = 60.0054 sec, min::max fragment size = 0.00202179::0.00208282 MB\n
// 4   Event counts: Run -- 1200271 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. \n
// 5 shm_nbb :10:1048576:0:0:10:0\n
// last parameter : communication/parsing error code
//-----------------------------------------------------------------------------
int get_receiver_data(const char* Url, float* Data) {

  int rc (0);

  xmlrpc_env    env;
  xmlrpc_value* resultP;

  struct ReceiverStatData_t {
                                        // line 0 doesn't have any numbers in it
    int    nEventsRead;                 // n(events) within the time window
    int    eventRate;                   // end of line
    float  dataRateEv;                  // MB/sec
    float  timeWindow;
    float  minEventSize;                // MB
    float  maxEventSize;                // MB , end of line 1
    float  elapsedTime;                 //      end of line 2

    int    nFragRead;                   // N(fragments) within the time window
    float  fragRate;
    float  dataRateFrag;                // MB/sec
    int    minFragSize;                 // per getNext call
    int    maxFragSize;                 // per getNext call , end of line 3

    int    nEvTotRun;                   // 
    int    nEvBadRun;                   // 
    int    nEvTotSubrun;                // 
    int    nEvBadSubrun;                // end of line 4

    int    nShmBufTot;                  // total number of allocated SHM buffers
    int    nShmBufEmpty;                // n empty (free) buffers
    int    nShmBufWrite;                // N buffers being written to 
    int    nShmBufFull;                 // N full buffers 
    int    nShmBufRead;                 // N buffers being read from
  } rs;

  xmlrpc_env_init(&env);
  memset(&rs,0,sizeof(ReceiverStatData_t));

                               // "({s:i,s:i})",
  TLOG(TLVL_INFO + 10) << "000: Url:" << Url;

  std::string res;

  try {
    resultP = xmlrpc_client_call(&env,Url,"daq.report","(s)","stats");
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

    TLOG(TLVL_INFO + 10) << "001: res:" << res;
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
//    Event statistics: 21309 events released at 355.118 events/sec, effective data rate = 0.733651 MB/sec, monitor window = 60.0054 sec, min::max event size = 0.00205994::0.00212097 MB\n
//-----------------------------------------------------------------------------
    boost::trim(lines[1]);
    TLOG(TLVL_INFO + 10) << "002: lines[1]:" << lines[1];

    vector<string> words;                                // words of the lines[0]
    boost::split(words,lines[1],boost::is_any_of(" "));
    rs.nEventsRead = std::stoi(words[ 2]);
    rs.eventRate   = std::stof(words[ 6]);
    rs.dataRateEv  = std::stof(words[12]);
    rs.timeWindow  = std::stof(words[17]);

    vector<string> num;
    boost::split(num,words[23],boost::is_any_of(":"));

    rs.minEventSize = std::stof(num[ 0]);
    rs.maxEventSize = std::stof(num[ 2]);
//-----------------------------------------------------------------------------
// line #2:
//    Average time per event:  elapsed time = 0.00281596 sec\n
//-----------------------------------------------------------------------------
    boost::trim(lines[2]);
    TLOG(TLVL_INFO + 10) << "003: lines[2]:" << lines[2];

    words.clear();                      // words of the lines[2]
    boost::split(words,lines[2],boost::is_any_of(" "));
    TLOG(TLVL_INFO + 10) << "004: lines[2].words[8]:" << words[8];
    rs.elapsedTime  = std::stof(words[ 8]);
//-----------------------------------------------------------------------------
// line #3:
//    Fragment statistics: 21309 fragments received at 355.118 fragments/sec, effective data rate = 0.720105 MB/sec, monitor window = 60.0054 sec, min::max fragment size = 0.00202179::0.00208282 MB\n
//-----------------------------------------------------------------------------
    boost::trim(lines[3]);
    TLOG(TLVL_INFO + 10) << "005: lines[3]:" << lines[3];

    words.clear();                      // words of the lines[3]
    boost::split(words,lines[3],boost::is_any_of(" "));

    rs.nFragRead    = std::stoi(words[ 2]);
    rs.fragRate     = std::stof(words[ 6]);
    rs.dataRateFrag = std::stof(words[12]);

    num.clear();
    boost::split(num,words[23],boost::is_any_of(":"));
    TLOG(TLVL_INFO + 10) << "006: lines[3].words[23]:" << words[23] << " num[0]:" << num[0] << " num[2]:" << num[2];
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

    rs.minFragSize  = std::stof(num[ 0]);
    rs.maxFragSize  = std::stof(num[ 2]);
//-----------------------------------------------------------------------------
// line #4:
//    Event counts: Run -- 1200271 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. \n
//-----------------------------------------------------------------------------
    boost::trim(lines[4]);
    TLOG(TLVL_INFO + 10) << "lines[4]:" << lines[4];

    words.clear();                      // words of the lines[4]
    boost::split(words,lines[4],boost::is_any_of(" "));

    TLOG(TLVL_INFO + 10) << "lines[4].words[4]:" << words[4] << " words[6]:" << words[6] 
                         << " words[11]:" << words[11] << " words[13]:" << words[13] ;

    rs.nEvTotRun    = std::stoi(words[ 4]);
    rs.nEvBadRun    = std::stoi(words[ 6]);
    rs.nEvTotSubrun = std::stoi(words[11]);
    rs.nEvBadSubrun = std::stoi(words[13]);
//-----------------------------------------------------------------------------
// line #5:
// shm_nbb :10:1048576:0:0:10:0\n
//-----------------------------------------------------------------------------
    boost::trim(lines[5]);
    TLOG(TLVL_INFO + 10) << "lines[5]:" << lines[5];

    words.clear();                      // words of the lines[5]
    boost::split(words,lines[5],boost::is_any_of(":"));

    rs.nShmBufTot   = std::stoi(words[ 1]);
    rs.nShmBufEmpty = std::stoi(words[ 2]);
    rs.nShmBufWrite = std::stoi(words[ 3]);
    rs.nShmBufFull  = std::stoi(words[ 4]);
    rs.nShmBufRead  = std::stoi(words[ 5]);
  }
  catch (...) {
//-----------------------------------------------------------------------------
// any kind of error 
//-----------------------------------------------------------------------------
    TLOG(TLVL_ERROR) << "Url:" << Url << " response:" << res;
  }
//-----------------------------------------------------------------------------
// copy data to the output
//-----------------------------------------------------------------------------
 DONE_PARSING_1:
  Data[ 0] = rs.nEventsRead;
  Data[ 1] = rs.eventRate;
  Data[ 2] = rs.dataRateEv;
  Data[ 3] = rs.timeWindow;
  Data[ 4] = rs.minEventSize;
  Data[ 5] = rs.maxEventSize;
  Data[ 6] = rs.elapsedTime;
  Data[ 7] = rs.nFragRead;
  Data[ 8] = rs.fragRate;
  Data[ 9] = rs.dataRateFrag;
  Data[10] = rs.minFragSize;
  Data[11] = rs.maxFragSize;
  Data[12] = rs.nEvTotRun;
  Data[13] = rs.nEvBadRun;
  Data[14] = rs.nEvTotSubrun;
  Data[15] = rs.nEvBadSubrun;
  Data[16] = rs.nShmBufTot;
  Data[17] = rs.nShmBufEmpty;
  Data[18] = rs.nShmBufWrite;
  Data[19] = rs.nShmBufFull;
  Data[20] = rs.nShmBufRead;

  Data[21] = rc;              // error code

  return rc;
}

//-----------------------------------------------------------------------------
// this is the function which reads the registers
// ----------------------------------------------------------------------------
INT tfm_driver_get(TFM_DRIVER_INFO* Info, INT Channel, float *Pvalue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// start with the first boardreader, the rest will come after that
// limit the readout frequency
//-----------------------------------------------------------------------------
  static time_t previous_timer(0);
  time_t        timer;
  const char*   default_trace_file = "/proc/trace/buffer";

  TLOG(TLVL_DEBUG+20) << "driver called";

  timer = time(NULL); 
  double time_diff = difftime(timer,previous_timer);
  if ((time_diff > 5) and (Channel == 0)) { 
    previous_timer=timer;
//-----------------------------------------------------------------------------
// debugging-only:
// sometimes it is important to make sure that one is looking at 
// the right TRACE file - there could be many
// have the name printed out
//-----------------------------------------------------------------------------
    const char* trace_file = std::getenv("TRACE_FILE");
    if (trace_file == nullptr) trace_file = default_trace_file;

    TLOG(TLVL_DEBUG) << "reading channel:" << Channel << "; TRACE_FILE=" << trace_file;
//-----------------------------------------------------------------------------
    if (Channel == 0) {
//-----------------------------------------------------------------------------
// read once for all channels
//-----------------------------------------------------------------------------
      get_boardreader_data(_brXmlrpcUrl.data(),&Info->array[ 0]);  // 30 parameters
      get_receiver_data   (_ebXmlrpcUrl.data(),&Info->array[40]);  // 22 parameters
      get_receiver_data   (_dlXmlrpcUrl.data(),&Info->array[70]);  // 22 parameters
//-----------------------------------------------------------------------------
// finally, get the output file information
// the script should return just one line with two numbers - time [ms] size [bytes]
//-----------------------------------------------------------------------------
      char buf[100];
      std::string res;

      Info->array[98] = 0;
      Info->array[99] = 0;
      TLOG(TLVL_DEBUG) << "before getting the output file size";
      try {
        char cmd[200];
        sprintf(cmd,"source $TFM_DIR/bin/tfm_configure %s %i > /dev/null; source daq_scripts/get_output_file_size",
                _artdaq_conf,_partition);
        TLOG(TLVL_DEBUG) << "before issuing cmd:" << cmd;
        FILE* pipe = popen(cmd, "r");
        while (!feof(pipe)) {
          char* s = fgets(buf, 100, pipe);
          if (s) res += buf;
        }
        pclose(pipe);

        TLOG(TLVL_DEBUG) << "get_output_file_size output:" << res;

        vector<string> w;
        boost::split(w,res,boost::is_any_of(" "));

        int nw=w.size();

        TLOG(TLVL_INFO + 10) << "w.size: " << nw;

        for (int i=0; i<nw; i++) {
          TLOG(TLVL_INFO + 10) << "i, w[i]: " << i << " " << w[i];
        }

        double time_sec  = std::stol(w[0])/1000.;

        TLOG(TLVL_INFO + 10) << "time_sec: " << time_sec;

        double sz_mbytes = std::stoll(w[1])/1024./1024.;

        TLOG(TLVL_INFO + 10) << "sz_mbytes: " << sz_mbytes;

        Info->array[98]   = sz_mbytes;

        if (_prev_time_sec > 0) {
          Info->array[99] = (sz_mbytes-_prev_sz_mbytes)/(time_sec-_prev_time_sec+1.e-12);
        }

        _prev_sz_mbytes = sz_mbytes;
        _prev_time_sec  = time_sec;
      }
      catch (...) {
        TLOG(TLVL_ERROR) << "failed to get the DAQ output file size";
      }
    }
  }
//-----------------------------------------------------------------------------
// so far, 100 parameters (sparse) 
// Q: would it make sense to have a monitor frontend per executable type ???
//-----------------------------------------------------------------------------
  if (Channel < TFM_DRIVER_NWORDS) *Pvalue = Info->array[Channel];
  else {
    TLOG(TLVL_ERROR) << "channel = " << Channel <<" outside the limit of " << TFM_DRIVER_NWORDS << " . TROUBLE";
  }

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT tfm_driver(INT cmd, ...) {
   va_list         argptr;
   HNDLE           hKey;
   INT             channel, status;
   float           value  , *pvalue;
   TFM_DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT: {
      hKey = va_arg(argptr, HNDLE);
      TFM_DRIVER_INFO** pinfo = va_arg(argptr, TFM_DRIVER_INFO **);
      channel = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t *bd = va_arg(argptr, func_t *);
      status = tfm_driver_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, TFM_DRIVER_INFO *);
      status  = tfm_driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, TFM_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = tfm_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, TFM_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = tfm_driver_get(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
