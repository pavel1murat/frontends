/********************************************************************\

  Name:         nulldev.c
  Created by:   Stefan Ritt

  Contents:     NULL Device Driver. This file can be used as a 
                template to write a read device driver

  $Id$

\********************************************************************/
#include "TRACE/tracemf.h"
#define TRACE_NAME "dtc_driver"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"

#include "dtcInterfaceLib/DTC.h"
using namespace DTCLib; 

#include "dtc_frontend/dtc_driver.hh"

DTC* _dtc(nullptr);

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

#define DTC_DRIVER_SETTINGS_STR "\
Address = INT : 1\n\
DTC_ID  = INT : %i\n\
"

typedef INT(func_t) (INT cmd, ...);

//---- device driver routines --------------------------------------
// the init function creates a ODB record which contains the
// settings and initializes it variables as well as the bus driver
//-----------------------------------------------------------------------------
INT dtc_driver_init(HNDLE hkey, DTC_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int              status, size  ;
  HNDLE            hDB   , hkeydd;
  DTC_DRIVER_INFO  *info;

  TLOG(TLVL_INFO+10) << "001 channels:" << channels;

   /* allocate info structure */
  info = (DTC_DRIVER_INFO*) calloc(1, sizeof(DTC_DRIVER_INFO));
  *pinfo = info;

  cm_get_experiment_database(&hDB, NULL);
//-----------------------------------------------------------------------------
// create DTC_DRIVER settings record - what is that already exists ? - not overwritten?
// assume index = 0 corresponds to the PCIE card address=0
//------------------------------------------------------------------------------
  char str[1000];
  int dtc_pcie_address = 0;                                   // frontend_index % 2;
  sprintf(str,DTC_DRIVER_SETTINGS_STR,dtc_pcie_address);
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
// initialize DTC, assume that for some magic the ODB has the proper value of 
// dtcID stored in it
// ROC mask to be properly set....
//-----------------------------------------------------------------------------
  if (_dtc == nullptr) {
    printf(" DTC_ID = %i\n",info->driver_settings.dtcID);
    _dtc = new DTC(DTC_SimMode_NoCFO,info->driver_settings.dtcID,0x0,"");
  }

  if (!bd)                                                  return FE_ERR_ODB;

  /* initialize bus driver */
  status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
  
  TLOG(TLVL_INFO+10) << "010 EXIT status:" << status;

  if (status != SUCCESS)                                    return status;
  
  /* initialization of device, something like ... */
  BD_PUTS("init");
  
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT dtc_driver_exit(DTC_DRIVER_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT dtc_driver_set(DTC_DRIVER_INFO * info, INT channel, float value) {
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
INT dtc_driver_get(DTC_DRIVER_INFO * Info, INT Channel, float *PValue) {
   const int reg [4] = {0x9010, 0x9014, 0x9018, 0x901c}; 
   //   int rc;
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
   int dtc_id = Info->driver_settings.dtcID;
   TLOG(TLVL_INFO+10) << "001 DTC_ID:" << dtc_id << " Channel:" << Channel;
   /* rc = */ 
//-----------------------------------------------------------------------------
// channel=0: temperature, the rest three - voltages ; 
//            read all four registers, convert into the right units, and store in the internal buffer 
//-----------------------------------------------------------------------------
   if      (Channel == 0) {
     for (int i=0; i<4; i++) {
       _dtc->GetDevice()->read_register(reg[i],100,&val); 
       if (i == 0) Info->array[i] = (val/4096.)*503.975 - 273.15; // temperature
       else        Info->array[i] = (val/4095.)*3.;               // voltage
     }
   }

   if (Channel  < 4) *PValue = Info->array[Channel];
   else {
     TLOG(TLVL_ERROR) << "002 channel:" << Channel;
   }
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   TLOG(TLVL_INFO+10) << "010 dtc_id:" << dtc_id << " Channel:" << Channel << " PValue:" << *PValue;
//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
// and at this point implement sleep
// I don't need to read the DTC too often
//-----------------------------------------------------------------------------
   sleep(5);

   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT dtc_driver(INT cmd, ...) {
   va_list         argptr;
   HNDLE           hKey;
   INT             channel, status;
   float           value, *pvalue;
   DTC_DRIVER_INFO *info;

   va_start(argptr, cmd);
   status = FE_SUCCESS;

   TLOG(TLVL_INFO+10) << "001 cmd:" << cmd;

   switch (cmd) {
   case CMD_INIT: {
      hKey                    = va_arg(argptr, HNDLE);
      DTC_DRIVER_INFO** pinfo = va_arg(argptr, DTC_DRIVER_INFO **);
      channel                 = va_arg(argptr, INT);
      va_arg(argptr, DWORD);
      func_t* bd              = va_arg(argptr, func_t*);
      status                  = dtc_driver_init(hKey, pinfo, channel, bd);
//-----------------------------------------------------------------------------
// and now need to add some
//-----------------------------------------------------------------------------
      break;
   }
   case CMD_EXIT:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      status = dtc_driver_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);   // floats are passed as double
      status = dtc_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = dtc_driver_get(info, channel, pvalue);
      break;

   default:
      break;
   }

   va_end(argptr);

   TLOG(TLVL_INFO+10) << "010 EXIT status:" << status;
   return status;
}

/*------------------------------------------------------------------*/
