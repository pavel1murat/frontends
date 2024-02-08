///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// TFM DataReceiver (EB or DL) slow control driver : 22 words per data receiver
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define TRACE_NAME "tfm_disk_driver"

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
#include "tfm_frontend/tfm_disk_driver.hh"

using std::vector, std::string;

// static strings _fn("tfm_disk_driver.log");
// static FILE*  _f;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_DISK_DRIVER_SETTINGS_STR "\
Link     = INT    : 0\n\
Active   = INT    : 0\n\
HostName = STRING : \n\
"

typedef INT(func_t) (INT cmd, ...);

static char         _active_conf[100];
static int          _partition;

static double       _prev_time_sec (-1);
static double       _prev_sz_mbytes(-1);

static std::string  _xmlrpcUrl;       // XML-RPC url of the data receiver
//-----------------------------------------------------------------------------
// device driver routines
// the init function creates a ODB record which contains the
// settings and initialized it variables as well as the bus driver
// passed from midas/src/device_driver.cxx is the address of a pointer with 
// private info, to be allocated here... no information about the hostname
//-----------------------------------------------------------------------------
INT tfm_disk_driver_init(HNDLE hkey, TFM_DISK_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int                status, size;
  HNDLE              hDB   , hkeydd;
  TFM_DISK_DRIVER_INFO *info;
//-----------------------------------------------------------------------------
// allocate info structure
//-----------------------------------------------------------------------------
  info = (TFM_DISK_DRIVER_INFO*) calloc(1, sizeof(TFM_DISK_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);
//-----------------------------------------------------------------------------
// create DRIVER settings record 
//-----------------------------------------------------------------------------
  status = db_create_record(hDB, hkey, "DD", TFM_DISK_DRIVER_SETTINGS_STR);
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
  int sz   = sizeof(_active_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &_active_conf, &sz, TID_STRING, TRUE);

  HNDLE      h_active_conf;
  char       key[200];
  sprintf(key,"/Mu2e/RunConfigurations/%s",_active_conf);
	db_find_key(hDB, 0, key, &h_active_conf);

  sz = sizeof(int);
  db_get_value(hDB, h_active_conf, "ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
//-----------------------------------------------------------------------------
// the frontend monitors only ARTDAQ processes running on the same node with it
// this is convenient for book-keeping reasons
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name(host_name);
  sprintf(key,"/Mu2e/RunConfigurations/%s/DetectorConfiguration/DAQ/%s/Artdaq",_active_conf,host.data());
//-----------------------------------------------------------------------------
// need to figure which component this driver is monitoring 
// make sure it won't compile before that
// the driver name is the same as the component name
//-----------------------------------------------------------------------------
  strcpy(info->driver_settings.HostName,FrontendsGlobals::_driver->name);
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
INT tfm_disk_driver_exit(TFM_DISK_DRIVER_INFO * info) {
  info->bd(CMD_EXIT, info->bd_info);
  
  if (info->array) free(info->array);
  free(info);

  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT tfm_disk_driver_set(TFM_DISK_DRIVER_INFO * info, INT channel, float value) {
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
// this is the function which reads the registers
// ----------------------------------------------------------------------------
INT tfm_disk_driver_get(TFM_DISK_DRIVER_INFO* Info, INT Channel, float *PValue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// start with the first boardreader, the rest will come after that
// limit the readout frequency
//-----------------------------------------------------------------------------
  static time_t previous_timer(0);
  time_t        timer;
  const char*   default_trace_file = "/proc/trace/buffer";

  TLOG(TLVL_DEBUG) << "000: driver called";
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

    TLOG(TLVL_DEBUG) << "001: reading channel:" << Channel << "; TRACE_FILE=" << trace_file;
//-----------------------------------------------------------------------------
// get the output file information
// the get_output_file_size script should return just one line with two numbers - time [ms] size [bytes]
//-----------------------------------------------------------------------------
    char buf[100];
    std::string res;

    Info->array[0] = 0;
    Info->array[1] = 0;
    TLOG(TLVL_DEBUG) << "002: before getting the output file size";
    try {
      char cmd[200];
      sprintf(cmd,"source $TFM_DIR/bin/tfm_configure %s %i > /dev/null; source daq_scripts/get_output_file_size",
              _active_conf,_partition);
      TLOG(TLVL_DEBUG) << "003: before issuing cmd:" << cmd;
      FILE* pipe = popen(cmd, "r");
      while (!feof(pipe)) {
        char* s = fgets(buf, 100, pipe);
        if (s) res += buf;
      }
      pclose(pipe);
      
      TLOG(TLVL_DEBUG) << "004: get_output_file_size output:" << res;
      
      vector<string> w;
      boost::split(w,res,boost::is_any_of(" "));
      
      int nw=w.size();
      
      TLOG(TLVL_INFO + 10) << "005: w.size: " << nw;
      
      for (int i=0; i<nw; i++) {
        TLOG(TLVL_INFO + 10) << "006: i, w[i]: " << i << " " << w[i];
      }
      
      double time_sec  = std::stol(w[0])/1000.;
      double sz_mbytes = std::stoll(w[1])/1024./1024.;
      
      TLOG(TLVL_INFO + 10) << "007: time_sec: " << time_sec << "sz_mbytes: " << sz_mbytes;
      
      Info->array[0]   = sz_mbytes;
      
      if (_prev_time_sec > 0) {
        Info->array[1] = (sz_mbytes-_prev_sz_mbytes)/(time_sec-_prev_time_sec+1.e-12);
      }
      
      _prev_sz_mbytes = sz_mbytes;
      _prev_time_sec  = time_sec;
    }
    catch (...) {
      TLOG(TLVL_ERROR) << "failed to get the DAQ output file size";
    }
  }
//-----------------------------------------------------------------------------
// so far, 100 parameters (sparse) 
// Q: would it make sense to have a monitor frontend per executable type ???
//-----------------------------------------------------------------------------
  if (Channel < TFM_DISK_DRIVER_NWORDS) *PValue = Info->array[Channel];
  else {
    TLOG(TLVL_ERROR) << "channel = " << Channel <<" outside the limit of " << TFM_DISK_DRIVER_NWORDS << " . TROUBLE";
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
INT tfm_disk_driver_get_label(TFM_DISK_DRIVER_INFO * Info, INT Channel, char* Label) {
  if      (Channel ==  0) sprintf(Label,"%s#file_size" ,Info->driver_settings.HostName);
  else if (Channel ==  1) sprintf(Label,"%s#io_rate"   ,Info->driver_settings.HostName);
  else {
    TLOG(TLVL_WARNING) << "channel:" << Channel << ". Do nothing";  
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// TFM data receiver (DR) slow control driver 
//-----------------------------------------------------------------------------
INT tfm_disk_driver(INT cmd, ...) {
   va_list             argptr;
   HNDLE               hKey;
   INT                 channel, status;
   float               value  , *pvalue;
   char*               label;
   TFM_DISK_DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT: {
      hKey = va_arg(argptr, HNDLE);
      TFM_DISK_DRIVER_INFO** pinfo = va_arg(argptr, TFM_DISK_DRIVER_INFO **);
      channel = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t *bd = va_arg(argptr, func_t *);
      status = tfm_disk_driver_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, TFM_DISK_DRIVER_INFO *);
      status  = tfm_disk_driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, TFM_DISK_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = tfm_disk_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, TFM_DISK_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = tfm_disk_driver_get(info, channel, pvalue);
      break;

  case CMD_GET_LABEL:
    info    = va_arg(argptr, TFM_DISK_DRIVER_INFO *);
    channel = va_arg(argptr, INT);
    label   = va_arg(argptr, char *);
    status  = tfm_disk_driver_get_label(info, channel, label);
    break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
