///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// TFM DataReceiver (EB or DL) slow control driver : 22 words per data receiver
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define TRACE_NAME "tfm_dr_driver"

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
#include "tfm_frontend/tfm_dr_driver.hh"

using std::vector, std::string;

// static strings _fn("tfm_dr_driver.log");
// static FILE*  _f;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_DR_DRIVER_SETTINGS_STR "\
Link      = INT : 0\n\
Active    = INT : 0\n\
CompName  = STRING : [32]\n\
XmlrpcUrl = STRING : [64]\n\
"

typedef INT(func_t) (INT cmd, ...);

static int          _partition;
//-----------------------------------------------------------------------------
// device driver routines
// the init function creates a ODB record which contains the
// settings and initialized it variables as well as the bus driver
// passed from midas/src/device_driver.cxx is the address of a pointer with 
// private info, to be allocated here... no information about the hostname
//-----------------------------------------------------------------------------
INT tfm_dr_driver_init(HNDLE hkey, TFM_DR_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int                status, size;
  HNDLE              hDB   , hkeydd;
  TFM_DR_DRIVER_INFO *info;
//-----------------------------------------------------------------------------
// allocate info structure
//-----------------------------------------------------------------------------
  info = (TFM_DR_DRIVER_INFO*) calloc(1, sizeof(TFM_DR_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);
//-----------------------------------------------------------------------------
// create DRIVER settings record 
//-----------------------------------------------------------------------------
  status = db_create_record(hDB, hkey, "DD", TFM_DR_DRIVER_SETTINGS_STR);
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
//-----------------------------------------------------------------------------
  char       active_conf[100];
  int sz   = sizeof(active_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &active_conf, &sz, TID_STRING, TRUE);

  HNDLE      h_active_conf;
  char       key[200];
  sprintf(key,"/Mu2e/RunConfigurations/%s",active_conf);
	db_find_key(hDB, 0, key, &h_active_conf);

  sz = sizeof(int);
  db_get_value(hDB, h_active_conf, "ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
//-----------------------------------------------------------------------------
// the frontend monitors only ARTDAQ processes running on the same node with it
// this is convenient for book-keeping reasons
//-----------------------------------------------------------------------------
  std::string hname=host_name;
  if (hname == "") hname = "local";

  std::string host = get_full_host_name(hname.data());
  sprintf(key,"/Mu2e/RunConfigurations/%s/DetectorConfiguration/DAQ/%s/Artdaq",active_conf,host.data());

  HNDLE h_artdaq_conf;
	if (db_find_key(hDB, 0, key, &h_artdaq_conf) != DB_SUCCESS) {
    TLOG(TLVL_ERROR) << "0012 no handle for:" << key << ", got:" << h_artdaq_conf;
    return FE_ERR_ODB;
  }
//-----------------------------------------------------------------------------
// need to figure which component this driver is monitoring 
// make sure it won't compile before that
// the driver name is the same as the component name
//-----------------------------------------------------------------------------
  strcpy(info->driver_settings.CompName,FrontendsGlobals::_driver->name);

  std::string  url;       // XML-RPC url of the data receiver
  get_xmlrpc_url(hDB,h_artdaq_conf,host.data(),_partition,FrontendsGlobals::_driver->name,url);
  strcpy(info->driver_settings.XmlrpcUrl,url.data());

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
// EXIT function of bus driver usually closes the device
//-----------------------------------------------------------------------------
INT tfm_dr_driver_exit(TFM_DR_DRIVER_INFO * info) {
  info->bd(CMD_EXIT, info->bd_info);
  
  if (info->array) free(info->array);
  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT tfm_dr_driver_set(TFM_DR_DRIVER_INFO * info, INT channel, float value) {
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
// parse data receiver (EB or DL) statistics report:
// ----------------------------------
// 0 datalogger01 statistics:\n
// 1   Event statistics: 21309 events released at 355.118 events/sec, effective data rate = 0.733651 MB/sec, monitor window = 60.0054 sec, min::max event size = 0.00205994::0.00212097 MB\n
// 2   Average time per event:  elapsed time = 0.00281596 sec\n
// 3   Fragment statistics: 21309 fragments received at 355.118 fragments/sec, effective data rate = 0.720105 MB/sec, monitor window = 60.0054 sec, min::max fragment size = 0.00202179::0.00208282 MB\n
// 4   Event counts: Run -- 1200271 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. \n
// 5 shm_nbb :10:1048576:0:0:10:0\n
// last parameter : communication/parsing error code
//-----------------------------------------------------------------------------
int get_receiver_data(TFM_DR_DRIVER_INFO* Info, float* Data) {

  int                rc(0);
  ReceiverStatData_t rs;

  xmlrpc_env         env;
  xmlrpc_value*      resultP;

  xmlrpc_env_init(&env);
  memset(&rs,0,sizeof(ReceiverStatData_t));

                               // "({s:i,s:i})",
  const char* url = Info->driver_settings.XmlrpcUrl;
  TLOG(TLVL_INFO + 10) << "000: url:" << url;

  std::string res;

  try {
    resultP = xmlrpc_client_call(&env,url,"daq.report","(s)","stats");
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
    TLOG(TLVL_ERROR) << "url:" << url << " response:" << res;
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
INT tfm_dr_driver_get(TFM_DR_DRIVER_INFO* Info, INT Channel, float *PValue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// start with the first boardreader, the rest will come after that
// limit the readout frequency
//-----------------------------------------------------------------------------
  static time_t previous_timer(0);
  time_t        timer;
  const char*   default_trace_file = "/proc/trace/buffer";

  TLOG(TLVL_DEBUG) << "driver called";
//-----------------------------------------------------------------------------
// the timing part is a kludge - need to learn how to handle this correctly
// however, this can be fixed transparently at any point in time
//-----------------------------------------------------------------------------
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
// read once for all channels
//-----------------------------------------------------------------------------
    get_receiver_data   (Info,&Info->array[0]);  // 22 parameters
  }
//-----------------------------------------------------------------------------
// so far, 100 parameters (sparse) 
// Q: would it make sense to have a monitor frontend per executable type ???
//-----------------------------------------------------------------------------
  if (Channel < TFM_DR_DRIVER_NWORDS) *PValue = Info->array[Channel];
  else {
    TLOG(TLVL_ERROR) << "channel = " << Channel <<" outside the limit of " << TFM_DR_DRIVER_NWORDS << " . TROUBLE";
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
INT tfm_dr_driver_get_label(TFM_DR_DRIVER_INFO * Info, INT Channel, char* Label) {
  if      (Channel ==  0) sprintf(Label,"%s#nev_read"    ,Info->driver_settings.CompName);
  else if (Channel ==  1) sprintf(Label,"%s#event_rate"  ,Info->driver_settings.CompName);
  else if (Channel ==  2) sprintf(Label,"%s#data_rate_ev",Info->driver_settings.CompName);
  else if (Channel ==  3) sprintf(Label,"%s#time_window" ,Info->driver_settings.CompName);
  else if (Channel ==  4) sprintf(Label,"%s#min_ev_size" ,Info->driver_settings.CompName);
  else if (Channel ==  5) sprintf(Label,"%s#max_ev_size" ,Info->driver_settings.CompName);
  else if (Channel ==  6) sprintf(Label,"%s#elapsed_time",Info->driver_settings.CompName);
  else if (Channel ==  7) sprintf(Label,"%s#nfrag_read"  ,Info->driver_settings.CompName);
  else if (Channel ==  8) sprintf(Label,"%s#frag_rate"   ,Info->driver_settings.CompName);
  else if (Channel ==  9) sprintf(Label,"%s#data_rate_fr",Info->driver_settings.CompName);
  else if (Channel == 10) sprintf(Label,"%s#min_fr_size" ,Info->driver_settings.CompName);
  else if (Channel == 11) sprintf(Label,"%s#max_fr_size" ,Info->driver_settings.CompName);
  else if (Channel == 12) sprintf(Label,"%s#nev_tot_run" ,Info->driver_settings.CompName);
  else if (Channel == 13) sprintf(Label,"%s#nev_bad_run" ,Info->driver_settings.CompName);
  else if (Channel == 14) sprintf(Label,"%s#nev_tot_sr"  ,Info->driver_settings.CompName);
  else if (Channel == 15) sprintf(Label,"%s#nev_bad_sr"  ,Info->driver_settings.CompName);
  else if (Channel == 16) sprintf(Label,"%s#nshm_buf_tot",Info->driver_settings.CompName);
  else if (Channel == 17) sprintf(Label,"%s#nshm_buf_emp",Info->driver_settings.CompName);
  else if (Channel == 18) sprintf(Label,"%s#nshm_buf_wr" ,Info->driver_settings.CompName);
  else if (Channel == 19) sprintf(Label,"%s#nshm_buf_ful",Info->driver_settings.CompName);
  else if (Channel == 20) sprintf(Label,"%s#nshm_buf_rd" ,Info->driver_settings.CompName);
  else {
    TLOG(TLVL_WARNING) << "channel:" << Channel << ". Do nothing";  
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// TFM data receiver (DR) slow control driver 
//-----------------------------------------------------------------------------
INT tfm_dr_driver(INT cmd, ...) {
   va_list             argptr;
   HNDLE               hKey;
   INT                 channel, status;
   float               value  , *pvalue;
   char*               label;
   TFM_DR_DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT: {
      hKey = va_arg(argptr, HNDLE);
      TFM_DR_DRIVER_INFO** pinfo = va_arg(argptr, TFM_DR_DRIVER_INFO **);
      channel = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t *bd = va_arg(argptr, func_t *);
      status = tfm_dr_driver_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, TFM_DR_DRIVER_INFO *);
      status  = tfm_dr_driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, TFM_DR_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = tfm_dr_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, TFM_DR_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = tfm_dr_driver_get(info, channel, pvalue);
      break;

  case CMD_GET_LABEL:
    info    = va_arg(argptr, TFM_DR_DRIVER_INFO *);
    channel = va_arg(argptr, INT);
    label   = va_arg(argptr, char *);
    status  = tfm_dr_driver_get_label(info, channel, label);
    break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
