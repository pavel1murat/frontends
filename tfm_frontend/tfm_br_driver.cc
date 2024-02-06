///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// TFM slow control boardreader (BR) driver
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
#define TRACE_NAME "tfm_br_driver"

#include <time.h>  
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>

#include "midas.h"
#include "mfe.h"

#include <vector>
#include <boost/algorithm/string.hpp>

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "frontends/utils/utils.hh"
#include "tfm_frontend/tfm_br_driver.hh"

using std::vector, std::string;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_BR_DRIVER_SETTINGS_STR "\
link   = INT : 0\n\
active = INT : 0\n\
NodeName = STR : \"\"\
"

typedef INT(func_t) (INT cmd, ...);

static char        _artdaq_conf[50];
static int         _partition;
// static int         _base_port_number;
// static int         _br_rank;
// static int         _br_port_number;
// static std::string _brUrl;
static std::string  _xmlrpcUrl;       // XML-RPC url of the data receiver

/*---- device driver routines --------------------------------------*/
/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */
//-----------------------------------------------------------------------------
INT tfm_br_driver_init(HNDLE hkey, TFM_BR_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int                status, size;
  HNDLE              hDB   , hkeydd;
  TFM_BR_DRIVER_INFO *info;
//-----------------------------------------------------------------------------
// allocate private info structure (DEVICE_DRIVER::dd_info)
//-----------------------------------------------------------------------------
  info = (TFM_BR_DRIVER_INFO*) calloc(1, sizeof(TFM_BR_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);
//-----------------------------------------------------------------------------
// create DRIVER settings record 
//-----------------------------------------------------------------------------
  status = db_create_record(hDB, hkey, "DD", TFM_BR_DRIVER_SETTINGS_STR);
  if (status != DB_SUCCESS)                                 return FE_ERR_ODB;

  db_find_key(hDB, hkey, "DD", &hkeydd);
  size = sizeof(info->driver_settings);
  db_get_record(hDB, hkeydd, &info->driver_settings, &size, 0);
//------------------------------------------------------------------------------
// initialize driver 
// info->array seems to be an internal array, use it to read
//-----------------------------------------------------------------------------
  info->num_channels = channels;
  info->array        = (float*) calloc(channels, sizeof(float));
  info->bd           = bd;
  info->hkey         = hkey;
//-----------------------------------------------------------------------------
// now figure out waht needs to be initialized
//-----------------------------------------------------------------------------
  char        active_conf[100];
  int   sz = sizeof(active_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &active_conf, &sz, TID_STRING, TRUE);

  HNDLE      h_active_conf;
  char       key[200];
  sprintf(key,"/Mu2e/RunConfigurations/%s",active_conf);
	db_find_key(hDB, 0, key, &h_active_conf);

  sz = sizeof(int);
  db_get_value(hDB, h_active_conf, "ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
//-----------------------------------------------------------------------------
// find out the port numbers for the first boardreader, first event builder, 
// and first datalogger
//-----------------------------------------------------------------------------
  sz = sizeof(_artdaq_conf);
  db_get_value(hDB,h_active_conf,"ArtdaqConfiguration", &_artdaq_conf, &sz, TID_STRING, TRUE);
  TLOG(TLVL_INFO+10) << "001 artdaq_conf:" << _artdaq_conf;

  sprintf(key,"/Mu2e/ArtdaqConfigurations/%s",_artdaq_conf);
  std::string k1 = key;

  HNDLE h_artdaq_conf;
	if (db_find_key(hDB, 0, k1.data(), &h_artdaq_conf) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "0012 no handle for:" << k1 << ", got:" << h_artdaq_conf;
  }
//-----------------------------------------------------------------------------
// a node monitoring frontend is submitted separately on each node
// the node name should be defined in the driver settings
// the nodename could be short of the 'host_name', a global from mfe.h
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name(host_name);
  sprintf(key,"/Mu2e/ArtdaqConfigurations/%s/%s",_artdaq_conf,host.data());
  k1 = key;

  HNDLE h_artdaq_node;
	if (db_find_key(hDB, 0, k1.data(), &h_artdaq_node) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "no handle for:" << k1 << ", got" << h_artdaq_node;
  }
//-----------------------------------------------------------------------------
// need to figure which component this driver is monitoring 
// make sure it won't compile before that
// in principle, global variable host_name should be available here via mfe.h
// expect FrontendsGlobals::_driver->name to be br01, dl01, eb01 etc....
//-----------------------------------------------------------------------------
  get_xmlrpc_url(hDB,h_artdaq_node,_partition,FrontendsGlobals::_driver->name,_xmlrpcUrl);
//-----------------------------------------------------------------------------
// it looks that the 'bus driver' function should be defined no matter what.
//-----------------------------------------------------------------------------
  if (info->bd == nullptr)  return FE_ERR_ODB;

  /* initialize the bus driver */
  status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
  
  if (status != SUCCESS)                                    return status;
  
  /* initialization of device, something like ... */
  BD_PUTS("init");

  //  _f = fopen(_fn.data(),"w");
  
  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// call EXIT function of bus driver, usually closes device
//-----------------------------------------------------------------------------
INT tfm_br_driver_exit(TFM_BR_DRIVER_INFO * info) {
  info->bd(CMD_EXIT, info->bd_info);
  
  if (info->array) free(info->array);
  free(info);

  //  fclose(_f);
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT tfm_br_driver_set(TFM_BR_DRIVER_INFO * info, INT channel, float value) {
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
int tfm_br_get_boardreader_data(const char* Url, float* Data) {
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

  std::string res;
  int         nf(-1);

  try {
    xmlrpc_env_init(&env);
                               // "({s:i,s:i})",
    resultP = xmlrpc_client_call(&env,Url,"daq.report","(s)","stats");
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
// parsing, normal ouutput should contain 6 lines
//-----------------------------------------------------------------------------
    vector<string> lines;
    boost::split(lines,res,boost::is_any_of("\n"));
    int nlines = lines.size();
    TLOG(TLVL_INFO + 10) << "001: nlines:" << nlines;
    if (nlines < 6) {
      rc = -10-nlines;
      TLOG(TLVL_ERROR) << "002 ERROR: nlines=" << nlines;
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

  if (nf <= 5) {
    for (int i=0; i<nf; i++) {
      Data[15+3*i] = brs.fragID  [i];
      Data[16+3*i] = brs.nFragsID[i];
      Data[17+3*i] = brs.nBytesID[i];
    }
  }
  else {
    TLOG(TLVL_ERROR) << "010: nf=" << nf;
  }
//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
  Data[30] = rc;

  TLOG(TLVL_INFO + 10) << "Data:" << Data[0] << " " << Data[1] << " rc=" << rc;
 
  return rc;
}

//-----------------------------------------------------------------------------
// this is the function which reads the registers
// ----------------------------------------------------------------------------
INT tfm_br_driver_get(TFM_BR_DRIVER_INFO* Info, INT Channel, float *Pvalue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// start with the first boardreader, the rest will come after that
// limit the readout frequency
//-----------------------------------------------------------------------------
  static time_t previous_timer(0);
  time_t        timer;
  const char*   default_trace_file = "/proc/trace/buffer";

  TLOG(TLVL_DEBUG+20) << "000: driver called";

  timer = time(NULL); 
  double time_diff = difftime(timer,previous_timer);
  if ((time_diff > 5) and (Channel == 0)) { 
    previous_timer = timer;
//-----------------------------------------------------------------------------
// debugging-only:
// sometimes it is important to make sure that one is looking at 
// the right TRACE file - there could be many
// have the name printed out
//-----------------------------------------------------------------------------
    const char* trace_file = std::getenv("TRACE_FILE");
    if (trace_file == nullptr) trace_file = default_trace_file;

    TLOG(TLVL_DEBUG) << "001: reading channel:" << Channel << " TRACE_FILE=" << trace_file;
//-----------------------------------------------------------------------------
// in a rush, the component names were hardcoded 
// need to extract them from ODB 
// it might make sense to have everything on a per-component basis,
// so there would be a driver for the boardreader and a separate driver 
// the event builder/data logger which format report is different 
//-----------------------------------------------------------------------------
    if (Channel == 0) {
//-----------------------------------------------------------------------------
// read once for all channels
//-----------------------------------------------------------------------------
      tfm_br_get_boardreader_data(_xmlrpcUrl.data(),Info->array);  // 29 parameters
    }
  }
//-----------------------------------------------------------------------------
// so far, 30 parameters (sparse) 
// Q: would it make sense to have a monitor frontend per executable type ???
//-----------------------------------------------------------------------------
  if (Channel < TFM_BR_DRIVER_NWORDS) *Pvalue = Info->array[Channel];
  else {
    TLOG(TLVL_ERROR) << "channel = " << Channel <<" outside the limit of " << TFM_BR_DRIVER_NWORDS << " . TROUBLE";
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// device driver entry point
//-----------------------------------------------------------------------------
INT tfm_br_driver(INT cmd, ...) {
  va_list         argptr;
  HNDLE           hKey;
  INT             channel, status;
  float           value  , *pvalue;
  TFM_BR_DRIVER_INFO *info;

  va_start(argptr, cmd);
  status = FE_SUCCESS;

  switch (cmd) {
  case CMD_INIT: {
    hKey       = va_arg(argptr, HNDLE);
    TFM_BR_DRIVER_INFO** pinfo = va_arg(argptr, TFM_BR_DRIVER_INFO **);
    channel    = va_arg(argptr, INT);
    va_arg(argptr, DWORD);
    func_t *bd = va_arg(argptr, func_t *);
    status     = tfm_br_driver_init(hKey, pinfo, channel, bd);
    break;
  }
  case CMD_EXIT:
    info    = va_arg(argptr, TFM_BR_DRIVER_INFO *);
    status  = tfm_br_driver_exit(info);
    break;
  case CMD_SET:
    info    = va_arg(argptr, TFM_BR_DRIVER_INFO *);
    channel = va_arg(argptr, INT);
    value   = (float) va_arg(argptr, double);   // floats are passed as double
    status  = tfm_br_driver_set(info, channel, value);
    break;
  case CMD_GET:
    info    = va_arg(argptr, TFM_BR_DRIVER_INFO *);
    channel = va_arg(argptr, INT);
    pvalue  = va_arg(argptr, float *);
    status  = tfm_br_driver_get(info, channel, pvalue);
    break;
  default:
    break;
  }

  va_end(argptr);

  return status;
}
