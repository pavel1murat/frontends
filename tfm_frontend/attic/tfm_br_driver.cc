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

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "tfm_frontend/tfm_br_driver.hh"

using std::vector, std::string;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_BR_DRIVER_SETTINGS_STR "\
Link           = INT : 0\n\
Active         = INT : 0\n\
NFragmentTypes = INT : 1\n\
CompName       = STRING : [32]\n\
XmlrpcUrl      = STRING : [64] :\n\
"

typedef INT(func_t) (INT cmd, ...);

static int         _partition;
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
  
  OdbInterface* odb_i           = OdbInterface::Instance(hDB);
  HNDLE       h_active_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string active_run_conf   = odb_i->GetRunConfigName(h_active_run_conf);
  std::string private_subnet    = odb_i->GetPrivateSubnet(h_active_run_conf);
  std::string public_subnet     = odb_i->GetPublicSubnet (h_active_run_conf);
  _partition                    = odb_i->GetArtdaqPartition();
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
// now figure out what needs to be initialized
// find out the port numbers for the first boardreader, first event builder, 
// and first datalogger
//
// short host name is a label like 'mu2edaq22'
// full host name is the name to be used to resolve the host IP and build the URLs
//-----------------------------------------------------------------------------
  std::string full_host_name = get_full_host_name (private_subnet.data());
  std::string host_label     = get_short_host_name(public_subnet.data());
  HNDLE h_host_artdaq_conf   = odb_i->GetHostArtdaqConfHandle(h_active_run_conf,host_label);
//-----------------------------------------------------------------------------
// need to figure which component this driver is monitoring 
// make sure it won't compile before that
// in principle, global variable host_name should be available here via mfe.h
// expect FrontendsGlobals::_driver->name to be br01, dl01, eb01 etc....
// the driver name is the same as the component name
//-----------------------------------------------------------------------------
  strcpy(info->driver_settings.CompName,FrontendsGlobals::_driver->name);

  std::string  xmlrpcUrl;       // XML-RPC url of the board reader
  get_xmlrpc_url(hDB,h_host_artdaq_conf,full_host_name.data(),_partition,FrontendsGlobals::_driver->name,xmlrpcUrl);

  TLOG(TLVL_INFO+10) << "013: host_label:" << host_label << " xmlRpcUrl:" << xmlrpcUrl;
  strcpy(info->driver_settings.XmlrpcUrl,xmlrpcUrl.data());
//-----------------------------------------------------------------------------
// for the boardreader, store the number of fragment types - that defines
// the number of lines in the  boardreader metrics report
// need to loop over components and find the one with a given label
//-----------------------------------------------------------------------------
  int   found(0);
  HNDLE h_component; 
  KEY   component;
  for (int i=0; db_enum_key(hDB, h_host_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
    db_get_key(hDB, h_component, &component);
    if (strcmp(FrontendsGlobals::_driver->name,component.name) == 0) {
      found = 1;
      break;
    }
  }

  if (found == 0) {
    TLOG(TLVL_ERROR) << "00121 no handle for key=" << FrontendsGlobals::_driver->name
                     << "in conf=" << active_run_conf;
  }

  int sz = sizeof(int);
  db_get_value(hDB, h_component, "NFragmentTypes", &info->driver_settings.NFragmentTypes, &sz, TID_INT32, FALSE);

  // at this point, update the driver record
  db_set_record(hDB, hkeydd, &info->driver_settings, size, 0);
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
//
// note that the number of lines in the boardreader response depends on the number of fragments on input
//------------------------------------------------------------------------------------------------------
int tfm_br_get_boardreader_data(TFM_BR_DRIVER_INFO* Info, float* Data) {
  // two words per process - N(segments/sec) and the data rate, MB/sec
  int           rc(0);
  xmlrpc_env    env;
  xmlrpc_value* resultP;

  BrStatData_t  brs;
  
  const char* url = Info->driver_settings.XmlrpcUrl;
  TLOG(TLVL_DEBUG+1) << "000: Url:" << url;
  memset(&brs,0,sizeof(BrStatData_t));

  std::string res;
  int         nf(-1);

  try {
    xmlrpc_env_init(&env);
                               // "({s:i,s:i})",
    resultP = xmlrpc_client_call(&env,url,"daq.report","(s)","stats");
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
    TLOG(TLVL_DEBUG+1) << "001: nlines:" << nlines;
    if (nlines < 4+Info->driver_settings.NFragmentTypes) {
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
    TLOG(TLVL_DEBUG+1) << "002: lines[0]:" << lines[0];

    vector<string> words;                                // words of the lines[0]
    boost::split(words,lines[0],boost::is_any_of(" ,"));

    TLOG(TLVL_DEBUG+1) << "003: lines[0].words[4]:" << words[4] << " lines[0].words[10]:" << words[10];
    brs.runNumber = std::stoi(words[ 4]);
    brs.nFragTot  = std::stoi(words[10]);
//-----------------------------------------------------------------------------
// second line: 
// 1:    Fragments read: 74828 fragments generated at 623.496 getNext calls/sec, fragment rate = 1246.99 fragments/sec, monitor window = 60.0068 sec, min::max read size = 2::2 fragments  Average times per fragment:  elapsed time = 0.00160386 sec\n
//-----------------------------------------------------------------------------
    boost::trim(lines[1]); 

    words.clear();
    boost::split(words,lines[1],boost::is_any_of(" "));

    TLOG(TLVL_DEBUG+1) << "004: lines[1]:" << lines[1];
    TLOG(TLVL_DEBUG+1) << "005: lines[1].nwords:" << words.size();

    brs.nFragRead    = std::stoi(words[ 2]);
    brs.getNextRate  = std::stof(words[ 6]);
    brs.fragRate     = std::stof(words[12]);
    brs.timeWindow   = std::stof(words[17]);
    
    vector<string> n1;
    boost::split(n1,words[23],boost::is_any_of(":"));

    TLOG(TLVL_DEBUG+1) << "006: lines[1],words[23]:" << words[23] << " n1.size():" << n1.size() << " n1[0]:" << n1[0] << " n1[2]:" << n1[2] ;
    
    brs.minNFrag    = std::stoi(n1[ 0]);
    brs.maxNFrag    = std::stoi(n1[ 2]);
    TLOG(TLVL_DEBUG+1) << "007: brs.minNFrag: " << brs.minNFrag << " brs.maxNFrag: " << brs.maxNFrag << " words[32]: " << words[32];
//-----------------------------------------------------------------------------
// it looks that there are some TAB characters hidden
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG+1) << "008: words[32]: " << words[32] << " words[33]: " << words[33];
    brs.elapsedTime = std::stof(words[33]);
    TLOG(TLVL_DEBUG+1) << "009: brs.elapsedTime: " << brs.elapsedTime;
//-----------------------------------------------------------------------------
// third line
// 2:    Fragment output statistics: 74827 fragments sent at 1246.97 fragments/sec, effective data rate = 0.584226 MB/sec, monitor window = 60.0068 sec, min::max event size = 0.000457764::0.000534058 MB\n
//-----------------------------------------------------------------------------
    boost::trim(lines[2]); 
    TLOG(TLVL_DEBUG+1) << "010: lines[2]:" << lines[2];

    words.clear();
    boost::split(words,lines[2],boost::is_any_of(" "));

    brs.dataRate    = std::stof(words[13]);

    vector<string> n2;
    boost::split(n2,words[24],boost::is_any_of(":"));
    brs.minEventSize = std::stof(n2[ 0]);
    brs.maxEventSize = std::stof(n2[ 2]);

    TLOG(TLVL_DEBUG+1) << "011: words[13]:" << words[13] << " words[24]:" << words[24] << " n2[0]:" << n2[0] << " n2[2]:" << n2[2];
//-----------------------------------------------------------------------------
// line # 4
//    Input wait time = 0.00156023 s/fragment, buffer wait time = 4.04943e-05 s/fragment, request wait time = 0.00075403 s/fragment, output wait time = 3.5861e-05 s/fragment\n
//-----------------------------------------------------------------------------
    boost::trim(lines[3]); 

    words.clear();
    boost::split(words,lines[3],boost::is_any_of(" "));
    TLOG(TLVL_DEBUG+1) << "012: words[4]:" << words[4] << " words[10]:" << words[10] 
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
        brs.fragID  [i] = std::stoi (num[ 1]);
      
        num.clear();
        boost::split(num,words[1],boost::is_any_of(":"));
        brs.nShmFragsID[i] = std::stoi (num[ 1]);

        num.clear();
        boost::split(num,words[2],boost::is_any_of(":"));
        brs.nShmBytesID[i] = std::stoi(num[ 1]);
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
  Data[ 0] = brs.runNumber;        // 
  Data[ 1] = brs.nFragTot ;        // 
  Data[ 2] = brs.nFragRead;        // 
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
  Data[14] = brs.requestWaitTime;
  Data[15] = brs.outputWaitTime;
//-----------------------------------------------------------------------------
// shared memory
//-----------------------------------------------------------------------------
  if (nf <= 5) {
    for (int i=0; i<nf; i++) {
      Data[16+i] = brs.fragID  [i];
      Data[21+i] = brs.nShmFragsID[i];
      Data[26+i] = brs.nShmBytesID[i];
    }
  }
  else {
    TLOG(TLVL_ERROR) << "010: nf=" << nf;
  }
//-----------------------------------------------------------------------------
// for i=4, the last word filled is Data[29]
//-----------------------------------------------------------------------------
  Data[30] = rc;

  TLOG(TLVL_DEBUG+1) << "Data[0]:" << Data[0] << " Data[1]" << Data[1] << " rc=" << rc;
 
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

  TLOG(TLVL_DEBUG+1) << "000: driver called";

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
// read once for all channels
//-----------------------------------------------------------------------------
    tfm_br_get_boardreader_data(Info,Info->array);  // 29 parameters
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
INT tfm_br_driver_get_label(TFM_BR_DRIVER_INFO * Info, INT Channel, char* Label) {
  if      (Channel ==  0) sprintf(Label,"%s#run_num"     ,Info->driver_settings.CompName);
  else if (Channel ==  1) sprintf(Label,"%s#nfrag_tot"   ,Info->driver_settings.CompName);
  else if (Channel ==  2) sprintf(Label,"%s#nfrag_read"  ,Info->driver_settings.CompName);
  else if (Channel ==  3) sprintf(Label,"%s#getnext_rate",Info->driver_settings.CompName);
  else if (Channel ==  4) sprintf(Label,"%s#frag_rate"   ,Info->driver_settings.CompName);
  else if (Channel ==  5) sprintf(Label,"%s#time_win"    ,Info->driver_settings.CompName);
  else if (Channel ==  6) sprintf(Label,"%s#min_nfrag"   ,Info->driver_settings.CompName);
  else if (Channel ==  7) sprintf(Label,"%s#max_nfrag"   ,Info->driver_settings.CompName);
  else if (Channel ==  8) sprintf(Label,"%s#elapsed_time",Info->driver_settings.CompName);
  else if (Channel ==  9) sprintf(Label,"%s#data_rate"   ,Info->driver_settings.CompName);
  else if (Channel == 10) sprintf(Label,"%s#min_ev_size" ,Info->driver_settings.CompName);
  else if (Channel == 11) sprintf(Label,"%s#max_ev_size" ,Info->driver_settings.CompName);
  else if (Channel == 12) sprintf(Label,"%s#inp_wtime"   ,Info->driver_settings.CompName);
  else if (Channel == 13) sprintf(Label,"%s#buf_wtime"   ,Info->driver_settings.CompName);
  else if (Channel == 14) sprintf(Label,"%s#req_wtime"   ,Info->driver_settings.CompName);
  else if (Channel == 15) sprintf(Label,"%s#out_wtime"   ,Info->driver_settings.CompName);
  else if (Channel == 16) sprintf(Label,"%s#frag_id_0"   ,Info->driver_settings.CompName);
  else if (Channel == 17) sprintf(Label,"%s#nfrag_0"     ,Info->driver_settings.CompName);
  else if (Channel == 18) sprintf(Label,"%s#nbytes_0"    ,Info->driver_settings.CompName);
  else if (Channel == 19) sprintf(Label,"%s#frag_id_1"   ,Info->driver_settings.CompName);
  else if (Channel == 20) sprintf(Label,"%s#nffrag_1"    ,Info->driver_settings.CompName);
  else if (Channel == 21) sprintf(Label,"%s#nbytes_1"    ,Info->driver_settings.CompName);
  else if (Channel == 22) sprintf(Label,"%s#frag_id_2"   ,Info->driver_settings.CompName);
  else if (Channel == 23) sprintf(Label,"%s#nffrag_2"    ,Info->driver_settings.CompName);
  else if (Channel == 24) sprintf(Label,"%s#nbytes_2"    ,Info->driver_settings.CompName);
  else if (Channel == 25) sprintf(Label,"%s#frag_id_3"   ,Info->driver_settings.CompName);
  else if (Channel == 26) sprintf(Label,"%s#nfrag_3"     ,Info->driver_settings.CompName);
  else if (Channel == 27) sprintf(Label,"%s#nbytes_3"    ,Info->driver_settings.CompName);
  else if (Channel == 28) sprintf(Label,"%s#frag_id_4"   ,Info->driver_settings.CompName);
  else if (Channel == 29) sprintf(Label,"%s#nfrag_4"     ,Info->driver_settings.CompName);
  else if (Channel == 30) sprintf(Label,"%s#nbytes_4"    ,Info->driver_settings.CompName);
  else {
    TLOG(TLVL_WARNING) << "channel:" << Channel << ". Do nothing";  
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
  char*           label;
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
  case CMD_GET_LABEL:
    info    = va_arg(argptr, TFM_BR_DRIVER_INFO *);
    channel = va_arg(argptr, INT);
    label   = va_arg(argptr, char *);
    status  = tfm_br_driver_get_label(info, channel, label);
    break;
  default:
    break;
  }

  va_end(argptr);

  return status;
}
