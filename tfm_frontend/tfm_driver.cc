///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.cc by Stefan Ritt
// ROC slow control driver
///////////////////////////////////////////////////////////////////////////////
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"

#include "frontends/tfm_frontend/tfm_driver.hh"

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

typedef struct {
  int link;
  int active;
} DRIVER_SETTINGS;

#define DRIVER_SETTINGS_STR "\
link   = INT : 0\n\
active = INT : 0\n\
"

/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  DRIVER_SETTINGS  driver_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} DRIVER_INFO;

typedef INT(func_t) (INT cmd, ...);

/*---- device driver routines --------------------------------------*/
/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */
//-----------------------------------------------------------------------------
INT driver_init(HNDLE hkey, DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int           status, size;
  HNDLE         hDB, hkeydd;
  DRIVER_INFO *info;

   /* allocate info structure */
  info = (DRIVER_INFO*) calloc(1, sizeof(DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

   /* create DRIVER settings record */
  status = db_create_record(hDB, hkey, "DD", DRIVER_SETTINGS_STR);
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
  
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT driver_exit(DRIVER_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);

  //   delete _dtc;
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT driver_set(DRIVER_INFO * info, INT channel, float value) {
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
// this is the function which reads the registers
// ----------------------------------------------
// ROC: all parameters come as one block read, assume that always real all of them
//-----------------------------------------------------------------------------
INT driver_get(DRIVER_INFO* Info, INT Channel, float *Pvalue) {
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT driver(INT cmd, ...) {
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT: {
      hKey = va_arg(argptr, HNDLE);
      DRIVER_INFO** pinfo = va_arg(argptr, DRIVER_INFO **);
      channel = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t *bd = va_arg(argptr, func_t *);
      status = driver_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, DRIVER_INFO *);
      status  = driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = driver_get(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
