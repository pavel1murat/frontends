///////////////////////////////////////////////////////////////////////////////
// when the execution comes here,  FrontendsGlobals::_driver points to the driver
// being initialized
//
//  Name:         nulldev.c
//  Created by:   Stefan Ritt
//
//  Contents:     NULL Device Driver. This file can be used as a 
//                template to write a read device driver
//
//  $Id$
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define TRACE_NAME "cfo_mon_driver"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "dtcInterfaceLib/DTC.h"
using namespace DTCLib; 

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"

#include "cfo_frontend/cfo_mon_driver.hh"

DTC* _dtc(nullptr);

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

typedef INT(func_t) (INT cmd, ...);

//---- device driver routines --------------------------------------
// the init function creates a ODB record which contains the
// settings and initializes it variables as well as the bus driver
//-----------------------------------------------------------------------------
INT cfo_driver_init(HNDLE hkey, CFO_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int              status, size  ;
  HNDLE            hDB   , hkeydd;
  std::string      active_run_conf;

  CFO_DRIVER_INFO  *info;

  TLOG(TLVL_INFO+10) << "001 channels:" << channels;
//-----------------------------------------------------------------------------
// allocate info structure, that includes DTC_DRIVER_SETTINGS
//-----------------------------------------------------------------------------
  info = (CFO_DRIVER_INFO*) calloc(1, sizeof(CFO_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);
  OdbInterface* odb_i = OdbInterface::Instance(hDB);

  HNDLE h_active_run_conf   = odb_i->GetActiveRunConfigHandle();
  active_run_conf           = odb_i->GetRunConfigName(h_active_run_conf);
  HNDLE       h_cfo_conf    = odb_i->GetCFOConfHandle(h_active_run_conf);

  int         external      = odb_i->GetCFOExternal       (hDB,h_cfo_conf);
  int         n_ewm_per_sec = odb_i->GetCFONEventsPerTrain(hDB,h_cfo_conf);
  int         pcie_addr     = odb_i->GetPcieAddress       (hDB,h_cfo_conf);
//-----------------------------------------------------------------------------
// create DTC_DRIVER settings record - what is that already exists ? - not overwritten?
// assume index = 0 corresponds to the PCIE card address=0
//------------------------------------------------------------------------------
  char str[1000];

  sprintf(str,CFO_DRIVER_SETTINGS_STR,pcie_addr,external,n_ewm_per_sec);
  status = db_create_record(hDB, hkey, "DD", str);

  if (status != DB_SUCCESS)                                 return FE_ERR_ODB;

  db_find_key(hDB, hkey, "DD", &hkeydd);
  size = sizeof(info->driver_settings);
  db_get_record(hDB, hkeydd, &info->driver_settings, &size, 0);

   /* initialize driver */
  info->num_channels = channels;
  info->array        = (float*) calloc(channels, sizeof(float));
  info->bd           = bd;
  info->hkey         = hkey;
//-----------------------------------------------------------------------------
// initialize CFO, assume that for some magic the ODB has the proper value of 
// dtcID stored in it
// force dtc_id to come from the driver name
//-----------------------------------------------------------------------------

  /* initialize bus driver */
  status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
  
  TLOG(TLVL_INFO+10) << "010 EXIT status:" << status;

  if (status != SUCCESS)                                    return status;
  
  /* initialization of device, something like ... */
  BD_PUTS("init");
  
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT cfo_driver_exit(CFO_DRIVER_INFO * info) {

  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  if (info->array) free(info->array);
  free(info);
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT cfo_driver_set(CFO_DRIVER_INFO * info, INT channel, float value) {
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
INT cfo_driver_get_label(CFO_DRIVER_INFO * Info, INT Channel, char* Label) {
  if      (Channel == 0) sprintf(Label,"CFO#nev");
  else {
    TLOG(TLVL_ERROR) << "channel:" << Channel << ". Do nothing";  
  }

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
INT cfo_driver_get(CFO_DRIVER_INFO * Info, INT Channel, float *PValue) {

  //  uint32_t val(0);

  TLOG(TLVL_DEBUG+10) << "cfo_driver_get: Channel: " << Channel;

  if      (Channel == 0) {
    cm_get_experiment_database(&hDB, NULL);
    char     key[100];
    HNDLE    h_cfo;

    sprintf(key,"/Equipment/CFO/nev");
    db_find_key(hDB, 0, key, &h_cfo);

    Info->array[0] = Info->array[0]+1;                // nevents
  }

  *PValue = Info->array[0];
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   TLOG(TLVL_INFO+10) << "010 cfo:" << " PValue:" << *PValue;
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// and at this point implement sleep
// I don't need to read the DTC too often
//-----------------------------------------------------------------------------
   sleep(1);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT cfo_mon_driver(INT cmd, ...) {
   va_list         argptr;
   HNDLE           hKey;
   INT             channel, status;
   float           value, *pvalue;
   char*           label;
   CFO_DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   TLOG(TLVL_INFO+10) << "001 cmd:" << cmd;

   switch (cmd) {
   case CMD_INIT: {
      hKey                    = va_arg(argptr, HNDLE);
      CFO_DRIVER_INFO** pinfo = va_arg(argptr, CFO_DRIVER_INFO **);
      channel                 = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t* bd              = va_arg(argptr, func_t*);
      status                  = cfo_driver_init(hKey, pinfo, channel, bd);
//-----------------------------------------------------------------------------
// and now need to add some
//-----------------------------------------------------------------------------
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, CFO_DRIVER_INFO *);
      status  = cfo_driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, CFO_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = cfo_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, CFO_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = cfo_driver_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info    = va_arg(argptr, CFO_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      label   = va_arg(argptr, char *);
      status  = cfo_driver_get_label(info, channel, label);
      break;

   default:
      break;
   }

   va_end(argptr);

   TLOG(TLVL_INFO+10) << "010 EXIT status:" << status;
   return status;
}
