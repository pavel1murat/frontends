/********************************************************************\

  Name:         driver.c
  Created by:   Stefan Ritt

  Contents:     NULL Device Driver. This file can be used as a 
                template to write a read device driver

  $Id$

\********************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"

#include "dtcInterfaceLib/DTC.h"

using namespace DTCLib; 

namespace {
  DTC* gDtc(nullptr);
}

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

typedef struct {
   int address;
} DRIVER_SETTINGS;

#define DRIVER_SETTINGS_STR "\
Address = INT : 1\n\
"

/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  DRIVER_SETTINGS driver_settings;
  float           *array;
  INT             num_channels;
  INT             (*bd) (INT cmd, ...); // bus driver entry function
  void            *bd_info;             // private info of bus driver
  HNDLE           hkey;                 // ODB key for bus driver info
} DRIVER_INFO;

/*---- device driver routines --------------------------------------*/

typedef INT(func_t) (INT cmd, ...);

/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */

//-----------------------------------------------------------------------------
// start from DTC=1, figure out the configuration side later
//-----------------------------------------------------------------------------
INT driver_init(HNDLE hkey, DRIVER_INFO **pinfo, INT channels, func_t *bd) {
   int status, size;
   HNDLE hDB, hkeydd;
   DRIVER_INFO *info;

   int const DTC_ID=1;

   if (gDtc == nullptr) {
     gDtc = new DTC(DTC_SimMode_NoCFO,DTC_ID,0x0,"");
   }

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

   /* initialize driver */
   info->num_channels = channels;
   info->array = (float*) calloc(channels, sizeof(float));
   info->bd = bd;
   info->hkey = hkey;

   if (!bd) return FE_ERR_ODB;

   /* initialize bus driver */
   status = info->bd(CMD_INIT, info->hkey, &info->bd_info);

   if (status != SUCCESS) return status;

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

   return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// for now, don't write into the DTC registers, but that could be useful!
//-----------------------------------------------------------------------------
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
// ch#0: reg 0x9010 : FPGA temperature    : T(C) = (ADC code)*503.975/4096 - 273.15
// ch#1: reg 0x9014 : FPGA VCCINT voltage : V(V) = (ADC code)/4095*3.
// ch#2: reg 0x9018 : FPGA VCCAUX voltage : V(V) = (ADC code)/4095*3.
// ch#3: reg 0x901c : FPGA VCBRAM voltage : V(V) = (ADC code)/4095*3.
//-----------------------------------------------------------------------------
INT driver_get(DRIVER_INFO * info, INT channel, float *pvalue) {
   uint32_t val(0);
//-----------------------------------------------------------------------------
// FPGA temperature
//-----------------------------------------------------------------------------
   val = gDtc->GetDevice()->read_register(0x9010,100,&val); 
   pvalue[0] = (val*503.975)/4096 - 273.15;
//-----------------------------------------------------------------------------
// voltages
//-----------------------------------------------------------------------------
   val = gDtc->GetDevice()->read_register(0x9014,100,&val); 
   pvalue[1] = (val/4095.)*3.;
   val = gDtc->GetDevice()->read_register(0x9018,100,&val); 
   pvalue[2] = (val/4095.)*3.;
   val = gDtc->GetDevice()->read_register(0x901c,100,&val); 
   pvalue[3] = (val/4095.)*3.;
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
      info = va_arg(argptr, DRIVER_INFO *);
      status = driver_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);   // floats are passed as double
      status = driver_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = driver_get(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
