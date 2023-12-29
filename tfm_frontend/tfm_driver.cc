///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// ROC slow control driver
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define TRACE_NAME "tfm_driver"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"

#include <vector>
#include <boost/algorithm/string.hpp>

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tfm_frontend/tfm_driver.hh"

using std::vector, std::string;

// static string _fn("tfm_driver.log");
// static FILE*  _f;

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define TFM_DRIVER_SETTINGS_STR "\
link   = INT : 0\n\
active = INT : 0\n\
"

typedef INT(func_t) (INT cmd, ...);

/*---- device driver routines --------------------------------------*/
/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */
//-----------------------------------------------------------------------------
INT tfm_driver_init(HNDLE hkey, TFM_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int             status, size;
  HNDLE           hDB   , hkeydd;
  TFM_DRIVER_INFO *info;

   /* allocate info structure */
  info = (TFM_DRIVER_INFO*) calloc(1, sizeof(TFM_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

   /* create DRIVER settings record */
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
//-----------------------------------------------------------------------------
// initialize DTC=1
//-----------------------------------------------------------------------------
  // if (_dtc == nullptr) {
  //    int roc_mask = 1 << (4*_link);
  //    _dtc = new DTC(DTC_SimMode_NoCFO,_dtcID,roc_mask,"");
  // }

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

  /* set channel to a specific value, something like ... */
  sprintf(str, "SET %d %lf", channel, value);
  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);

  /* simulate writing by storing value in local array, has to be removed
     in a real driver */
  if (channel < info->num_channels)
    info->array[channel] = value;

  return FE_SUCCESS;
}


//-----------------------------------------------------------------------------
int get_boardreader_data(const char* Url, float* Data) {
  // two words per process - N(segments/sec) and the data rate, MB/sec

  xmlrpc_env    env;
  xmlrpc_value* resultP;

  xmlrpc_env_init(&env);
                               // "({s:i,s:i})",
  resultP = xmlrpc_client_call(&env,Url,"daq.report","(s)","stats");
  // die_if_fault_occurred(&env);
  if (env.fault_occurred) {
    fprintf(stderr, "XML-RPC Fault: %s (%d)\n",env.fault_string, env.fault_code);
    exit(1);
  }
    // }


  const char* value;
  size_t      length;
  xmlrpc_read_string_lp(&env, resultP, &length, &value);
    
  std::string res = value;
  //  fprintf(_f,"%s",res.data());
//-----------------------------------------------------------------------------
//  now the output string needs to be parsed. it should look as follows
//  boardreader01 run number = 55, Sent Fragment count = 2308boardreader01 statistics:\n
//    Fragments read: 599 fragments generated at 9.98229 getNext calls/sec, fragment rate = 9.98229 fragments/sec, monitor window = 60.0063 sec, min::max read size = 1::1 fragments  Average times per fragment:  elapsed time = 0.100177 sec\n
//    Fragment output statistics: 599 fragments sent at 9.98229 fragments/sec, effective data rate = 0.00121854 MB/sec, monitor window = 60.0063 sec, min::max event size = 0.00012207::0.00012207 MB\n
//    Input wait time = 0.100128 s/fragment, buffer wait time = 3.90003e-05 s/fragment, request wait time = 0.100101 s/fragment, output wait time = 5.47901e-05 s/fragment
//-----------------------------------------------------------------------------
  vector<string> s1;
  boost::split(s1,res,boost::is_any_of("\n"));

  // now, split the second line by ',' : 
  vector<string> s2;
  boost::split(s2,s1[1],boost::is_any_of(","));

  // at this point, expect s2[1]:  'fragment rate = 9.98229 fragments/sec' 
  vector<string> s3;
  boost::split(s3,s2[1],boost::is_any_of("="));

  // at this point, expect s3[1]:  ' 9.98229 fragments/sec'
  vector<string> s4;
  boost::split(s4,s3[1],boost::is_any_of(" "));

  // s4[1] = '9.98229'

  float f = std::stof(s4[1]); // number of fragments per second, first boardreader

  Data[0] = f;
//-----------------------------------------------------------------------------
// now, split the 3rd line by ',' to extract the rate
//-----------------------------------------------------------------------------
  s2.clear();
  boost::split(s2,s1[2],boost::is_any_of(","));

  // at this point, expect s2[1]:  'effective data rate = 0.00121854 MB/sec' 
  s3.clear();
  boost::split(s3,s2[1],boost::is_any_of("="));

  // at this point, expect s3[1]:  ' 0.00121854 MB/sec'
  s4.clear();
  boost::split(s4,s3[1],boost::is_any_of(" "));
  
  // s4[1] = '0.00121854'
  
  Data[1] = std::stof(s4[1]);

  TLOG(TLVL_INFO+10) << "Data:" << Data[0] << " " << Data[1];
  
  xmlrpc_DECREF   (resultP);

  return 0;
}

//-----------------------------------------------------------------------------
int get_datalogger_data(const char* Url, float* Data) {
  // two words per process - N(segments/sec) and the data rate, MB/sec

  xmlrpc_env    env;
  xmlrpc_value* resultP;

  xmlrpc_env_init(&env);
                               // "({s:i,s:i})",
  resultP = xmlrpc_client_call(&env,Url,"daq.report","(s)","stats");
  if (env.fault_occurred) {
    fprintf(stderr, "XML-RPC Fault: %s (%d)\n",env.fault_string, env.fault_code);
    exit(1);
  }

  const char* value;
  size_t      length;
  xmlrpc_read_string_lp(&env, resultP, &length, &value);
    
  std::string res = value;
  //  fprintf(_f,"%s",res.data());
//-----------------------------------------------------------------------------
//  now the output string needs to be parsed. it should look as follows
//  datalogger01 statistics:\n
//    Event statistics: 598 events released at 9.96565 events/sec, effective data rate = 0.0158907 MB/sec, monitor window = 60.0061 sec, min::max event size = 0.00159454::0.00159454 MB\n
//    Average time per event:  elapsed time = 0.100345 sec\n
//    Fragment statistics: 598 fragments received at 9.96565 fragments/sec, effective data rate = 0.0155105 MB/sec, monitor window = 60.0061 sec, min::max fragment size = 0.0015564::0.0015564 MB\n
//    Event counts: Run -- 953 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. \n
//-----------------------------------------------------------------------------
  TLOG(TLVL_INFO + 10) << "res:" << res;

  vector<string> s1;
  boost::split(s1,res,boost::is_any_of("\n"));

  // now, split the second line by ' ' : 
  vector<string> s2;
  boost::split(s2,s1[1],boost::is_any_of("\ "));

  TLOG(TLVL_INFO + 10) << "s1[1]:" << s1[1];
  TLOG(TLVL_INFO + 10) << "s2[8]:" << s2[8] << " s2[14]:" << s2[14];
  // there are leading spaces ! 
  Data[0] = std::stof(s2[ 8]);
  Data[1] = std::stof(s2[14]);

  TLOG(TLVL_INFO + 10) << "Data:" << Data[0] << " " << Data[1];

  xmlrpc_DECREF   (resultP);

  return 0;
}

//-----------------------------------------------------------------------------
// this is the function which reads the registers
// ----------------------------------------------
INT tfm_driver_get(TFM_DRIVER_INFO* Info, INT Channel, float *Pvalue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// start with the first boardreader, the rest will come after that
//-----------------------------------------------------------------------------
  std::string   br1Url("http://localhost:21100/RPC2");
  std::string   dl1Url("http://localhost:21104/RPC2");

  if (Channel == 0) {
    // float data[2];
//-----------------------------------------------------------------------------
// read once for all channels
//-----------------------------------------------------------------------------
    get_boardreader_data(br1Url.data(),&Info->array[0]);
    get_datalogger_data (dl1Url.data(),&Info->array[2]);

    TLOG(TLVL_DEBUG+10) << "Data:" << Info->array[0] << " " << Info->array[1] ;
  }

  if (Channel < 4) *Pvalue = Info->array[Channel];
  else {
    printf("driver_get TFM_DRIVER: channel = %i. IN TROUBLE\n",Channel);
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
