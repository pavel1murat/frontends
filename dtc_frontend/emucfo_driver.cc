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
#define TRACE_NAME "dtc_driver"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "midas.h"
#include "mfe.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
using namespace DTCLib; 
using namespace trkdaq; 

#include "utils/utils.hh"
#include "dtc_frontend/emucfo_driver.hh"

/*---- globals -----------------------------------------------------*/

#define DEFAULT_TIMEOUT 10000   /* 10 sec. */

namespace {
  uint64_t _firstEWTag;
  uint64_t _nEvents;
  int      _ewLength;            // event window length [in 25ns ticks]
};

typedef INT(func_t) (INT cmd, ...);

//---- device driver routines --------------------------------------
// the init function creates a ODB record which contains the
// settings and initializes it variables as well as the bus driver
//-----------------------------------------------------------------------------
INT emucfo_driver_init(HNDLE hkey, DTC_DRIVER_INFO **pinfo, INT channels, func_t *bd) {
  int              status, size  ;
  HNDLE            hDB   , hkeydd;
  DTC_DRIVER_INFO  *info;

  TLOG(TLVL_INFO+10) << "001 channels:" << channels;
//-----------------------------------------------------------------------------
// allocate info structure, that includes DTC_DRIVER_SETTINGS
//-----------------------------------------------------------------------------
  info = *pinfo; // (DTC_DRIVER_INFO*) calloc(1, sizeof(DTC_DRIVER_INFO));

  cm_get_experiment_database(&hDB, NULL);
//-----------------------------------------------------------------------------
// create DTC_DRIVER settings record - what is that already exists ? - not overwritten?
// assume index = 0 corresponds to the PCIE card address=0
//------------------------------------------------------------------------------
  char str[1000];

  DTC_DRIVER_SETTINGS* dds = &info->driver_settings;
  
  sprintf(str,DTC_DRIVER_SETTINGS_STR,dds->pcieAddr,dds->enabled);
  status = db_create_record(hDB, hkey, "DD", str);

  if (status != DB_SUCCESS)                                 return FE_ERR_ODB;

  // db_find_key(hDB, hkey, "DD", &hkeydd);
  // size = sizeof(info->driver_settings);
  // db_get_record(hDB, hkeydd, &info->driver_settings, &size, 0);

   /* initialize driver */
  info->num_channels = channels;
  info->array        = (float*) calloc(channels, sizeof(float));
  info->bd           = bd;
  info->hkey         = hkey;

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
INT emucfo_driver_exit(DTC_DRIVER_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT emucfo_driver_set(DTC_DRIVER_INFO * info, INT channel, float value) {
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
INT emucfo_driver_get_label(DTC_DRIVER_INFO * Info, INT Channel, char* Label) {
  if      (Channel == 0) sprintf(Label,"DTC%i#Temp"  ,Info->driver_settings.pcieAddr);
  else {
    TLOG(TLVL_ERROR) << "channel:" << Channel << ". Do nothing";  
  }

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
INT emucfo_driver_get(DTC_DRIVER_INFO * Info, INT Channel, float *PValue) {

  if (Info->driver_settings.enabled == 0) {
    *PValue = 0;
    return FE_SUCCESS;
  }
  
  int pcie_addr = Info->driver_settings.pcieAddr;
  DtcInterface* dtc_i = DtcInterface::Instance(pcie_addr);
  TLOG(TLVL_INFO+10) << "001 PCIE address:" << pcie_addr << " Channel:" << Channel;
//-----------------------------------------------------------------------------
// channel=0: (fake) generate window markers)
//-----------------------------------------------------------------------------
  if      (Channel == 0) {
    dtc_i->LaunchRunPlanEmulatedCfo(_ewLength,_nEvents+1,_firstEWTag);
    PValue = 0;
  }

  return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT emucfo_driver(INT cmd, ...) {
   va_list         argptr;
   HNDLE           hKey;
   INT             channel, status;
   float           value, *pvalue;
   char*           label;
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
      status                  = emucfo_driver_init(hKey, pinfo, channel, bd);
//-----------------------------------------------------------------------------
// and now need to add some
//-----------------------------------------------------------------------------
      break;
   }
   case CMD_EXIT:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      status = emucfo_driver_exit(info);
      break;

   case CMD_SET:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value = (float) va_arg(argptr, double);   // floats are passed as double
      status = emucfo_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info = va_arg(argptr, DTC_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue = va_arg(argptr, float *);
      status = emucfo_driver_get(info, channel, pvalue);
      break;

   case CMD_GET_LABEL:
      info    = va_arg(argptr, DTC_DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      label   = va_arg(argptr, char *);
      status  = emucfo_driver_get_label(info, channel, label);
      break;

   default:
      break;
   }

   va_end(argptr);

   TLOG(TLVL_INFO+10) << "010 EXIT status:" << status;
   return status;
}
