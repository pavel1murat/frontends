/********************************************************************\

  Name:         nulldev.c
  Created by:   Stefan Ritt

  Contents:     NULL Device Driver. This file can be used as a 
                template to write a read device driver

  $Id$

\********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"

#include "dtcInterfaceLib/DTC.h"

using namespace DTCLib; 

DTC* gDtc(nullptr);

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

/* Store any parameters the device driver needs in following 
   structure. Edit the NULLDEV_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

typedef struct {
   int address;
} NULLDEV_SETTINGS;

#define NULLDEV_SETTINGS_STR "\
Address = INT : 1\n\
"

/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  NULLDEV_SETTINGS nulldev_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} NULLDEV_INFO;

typedef INT(func_t) (INT cmd, ...);

/*---- device driver routines --------------------------------------*/
/* the init function creates a ODB record which contains the
   settings and initialized it variables as well as the bus driver */

INT nulldev_init(HNDLE hkey, NULLDEV_INFO **pinfo, INT channels, func_t *bd) {
  int           status, size;
  HNDLE         hDB, hkeydd;
  NULLDEV_INFO *info;

   /* allocate info structure */
  info = (NULLDEV_INFO*) calloc(1, sizeof(NULLDEV_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);

   /* create NULLDEV settings record */
  status = db_create_record(hDB, hkey, "DD", NULLDEV_SETTINGS_STR);
  if (status != DB_SUCCESS)
    return FE_ERR_ODB;

  db_find_key(hDB, hkey, "DD", &hkeydd);
  size = sizeof(info->nulldev_settings);
  db_get_record(hDB, hkeydd, &info->nulldev_settings, &size, 0);

   /* initialize driver */
  info->num_channels = channels;
  info->array = (float*) calloc(channels, sizeof(float));
  info->bd = bd;
  info->hkey = hkey;
//-----------------------------------------------------------------------------
// initialize DTC=1
//-----------------------------------------------------------------------------
  if (gDtc == nullptr) {
    int DTC_ID=1;
    gDtc = new DTC(DTC_SimMode_NoCFO,DTC_ID,0x0,"");
  }

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
INT nulldev_exit(NULLDEV_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT nulldev_set(NULLDEV_INFO * info, INT channel, float value) {
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
INT nulldev_get(NULLDEV_INFO * info, INT channel, float *pvalue) {
   const int reg [4] = {0x9010, 0x9014, 0x9018, 0x901c}; 
   int rc;
   uint32_t val(0);
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
   // char str[80];
   // sprintf(str, "GET %d", channel);
   // BD_PUTS(str);
   // BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);
//-----------------------------------------------------------------------------
// FPGA temperature
//-----------------------------------------------------------------------------
   rc = gDtc->GetDevice()->read_register(reg[channel],100,&val); 
//-----------------------------------------------------------------------------
// channel=0: temperature, the rest - voltages
//-----------------------------------------------------------------------------
   if      (channel == 0) *pvalue = (val/4096.)*503.975 - 273.15; // temperature
   else if (channel  < 4) *pvalue = (val/4095.)*3.;               // voltage
   else {
     printf("nulldev_get DTC::nulldev: channel = %i. IN TROUBLE\n",channel);
   }
   // printf("channel: %i val: %i pvalue[channel] = %10.3f\n",channel,val,*pvalue);
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT nulldev(INT cmd, ...) {
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   NULLDEV_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   switch (cmd) {
   case CMD_INIT: {
      hKey = va_arg(argptr, HNDLE);
      NULLDEV_INFO** pinfo = va_arg(argptr, NULLDEV_INFO **);
      channel = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t *bd = va_arg(argptr, func_t *);
      status = nulldev_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info = va_arg(argptr, NULLDEV_INFO *);
      status = nulldev_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, NULLDEV_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);   // floats are passed as double
      status = nulldev_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, NULLDEV_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = nulldev_get(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
