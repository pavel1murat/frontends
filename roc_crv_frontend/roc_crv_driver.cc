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

#include "artdaq-core-mu2e/Overlays/DTC_Types.h"
#include "dtcInterfaceLib/DTC.h"

#include "roc_crv_frontend/roc_crv_driver.hh"

using namespace DTCLib; 

int         _link(0);
DTC_Link_ID _roc          = DTC_Link_ID(_link);
int         _dtcID(0);  

int const   _sleepTimeDTC     ( 200);
int const   _sleepTimeROC     (2500);
int const   _sleepTimeROCReset(4000);
int const   _DCSTimeoutROC    (100);

DTC*        _dtc(nullptr);

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
INT roc_crv_driver_init(HNDLE hkey, DRIVER_INFO **pinfo, INT channels, func_t *bd) {
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
  if (_dtc == nullptr) {
     int roc_mask = 1 << (4*_link);
     _dtc = new DTC(DTC_SimMode_NoCFO,_dtcID,roc_mask,"");
  }

  if (!bd)  return FE_ERR_ODB;

  /* initialize bus driver */
  status = info->bd(CMD_INIT, info->hkey, &info->bd_info);
  
  if (status != SUCCESS)
    return status;
  
  /* initialization of device, something like ... */
  BD_PUTS("init");
 
  bd(CMD_DEBUG, TRUE);


  /* set up DTC for CRV ROC DCS */
  _dtc->GetDevice()->write_register(0x93e0,100,0x00400000); // ROC DCS Response Timer Preset
 
  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT roc_crv_driver_exit(DRIVER_INFO * info) {
  /* call EXIT function of bus driver, usually closes device */
  info->bd(CMD_EXIT, info->bd_info);
  
  /* free local variables */
  if (info->array)
    free(info->array);

  free(info);

  delete _dtc;
  
  return FE_SUCCESS;
}

/*----------------------------------------------------------------------------*/
INT roc_crv_driver_set(DRIVER_INFO * info, INT channel, float value) {
  const int adds[] = {0x1044, // FPGA 0, Bias 0
                      0x1045, // FPGA 0, Bias 0
                      0x1444, // FPGA 0, Bias 0
                      0x1445, // FPGA 0, Bias 0
                      0x1844, // FPGA 0, Bias 0
                      0x1845, // FPGA 0, Bias 0
                      0x1c44, // FPGA 0, Bias 0
                      0x1c45  // FPGA 0, Bias 0
                     };
   size_t adds_size = adds_size = sizeof(adds)/sizeof(adds[0]);

   int ivalue = static_cast<int>(value);
   if(channel < adds_size) {
       _dtc->WriteROCRegister(_roc, adds[channel], ivalue, false, 100);
   } else {
     // channel not yet implemented
   }


  /* Simon
   * Bypassing the Bus Driver because we use the _dtc here in the device 
   * driver. Our DTC class is the equivalent of a bus driver but I belive
   * its not worth to match the midas bus driver concept.
   */
   
  char str[80];
  
  /* set channel to a specific value, something like ... */
  sprintf(str, "SET %d %i", channel, ivalue);
  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);
  
  /* simulate writing by storing value in local array, has to be removed
     in a real driver */
  if (channel < info->num_channels)
    info->array[channel] = value;

  return FE_SUCCESS;
}


/*----------------------------------------------------------------------------*/
INT roc_crv_driver_set_register(DRIVER_INFO * info, INT add, int32_t value) {
  _dtc->WriteROCRegister(_roc, add, value, false, 100);

  char str[80];

  /* set channel to a specific value, something like ... */
  sprintf(str, "SET REGISTER %d %i", add, value);
  BD_PUTS(str);
  BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);

  return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// this is the function which reads the channels, which are apped registers
// ----------------------------------------------
INT roc_crv_driver_get(DRIVER_INFO* Info, INT Channel, float *Pvalue) {
   int32_t val;
 
   const int adds[] = {0x0035, // ROC counter
                       0x8004, // number of active FEBs
                       0x8021, // ROC port 1 voltage
                       0x8041, // ROC port 1 amps
                       0x8401, // FEB 1: serial number
                       0x8421, // FEB 1: spill cycle counter
                       0x8461, // FEB 1: temperature
                       0x8481, // FEB 1, ADC  0: 1.2v_Pos 
                       0x84a1, // FEB 1, ADC  1: 1.8v_Pos
                       0x84c1, // FEB 1, ADC  2: 5.0v_Pos
                       0x84e1, // FEB 1, ADC  3: 10v_Pos
                       0x8501, // FEB 1, ADC  4: 2.5v_Pos
                       0x8521, // FEB 1, ADC  5: 5.0v_Neg
                       0x8541, // FEB 1, ADC  6: 15v_Pos
                       0x8661, // SOMETHING IS OFF WITH THE ORDER?
                       0x8561, // FEB 1, ADC  7: 3.3v_Pos
                       0x8581, // FEB 1, ADC  8: Bias_0
                       0x85a1, // FEB 1, ADC  9: Bias_1
                       0x85c1, // FEB 1, ADC 10: Bias_2
                       0x85e1, // FEB 1, ADC 11: Bias_3
                       0x8601, // FEB 1, ADC 12: Bias_4
                       0x8621, // FEB 1, ADC 13: Bias_5
                       0x8641  // FEB 1, ADC 14: Bias_6
                       //0x8661  // FEB 1, ADC 15: Bias_7
                       };

   size_t adds_size = sizeof(adds)/sizeof(adds[0]);
   if(Channel < adds_size) {
      //for(size_t i=0; i < adds_size; ++i) {
          try { 
              val = _dtc->ReadROCRegister(_roc, adds[Channel], _DCSTimeoutROC);
              *Pvalue = val;
          } catch (const std::exception &exc) {
              cm_msg(MERROR, "roc_crv_driver_get", exc.what());
              *Pvalue = -1;
          }
       //}
   } else {
      *Pvalue = Channel;
   }

//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   return FE_SUCCESS;
}

//-----------------------------------------------------------------------------
// this is the function which reads  registers
// ----------------------------------------------
INT roc_crv_driver_get_register(DRIVER_INFO* Info, INT add, int32_t *Pvalue) {
  int32_t val = _dtc->ReadROCRegister(_roc, add, _DCSTimeoutROC);
  *Pvalue = val;

//-----------------------------------------------------------------------------
// assume success for now and implement handling of timeouts/errors etc later
//-----------------------------------------------------------------------------
   return FE_SUCCESS;
}

/*---- device driver entry point -----------------------------------*/

INT roc_crv_driver(INT cmd, ...) {
   va_list argptr;
   HNDLE hKey;
   INT channel, status;
   float value, *pvalue;
   int32_t valueint, *pvalueint;
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
      status = roc_crv_driver_init(hKey, pinfo, channel, bd);
      break;
   }
   case CMD_EXIT:
      info    = va_arg(argptr, DRIVER_INFO *);
      status  = roc_crv_driver_exit(info);
      break;

   case CMD_SET:
      info    = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      value   = (float) va_arg(argptr, double);   // floats are passed as double
      status  = roc_crv_driver_set(info, channel, value);
      break;

   case CMD_GET:
      info    = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalue  = va_arg(argptr, float *);
      status  = roc_crv_driver_get(info, channel, pvalue);
      break;

   case CMD_SET+1000: // register, not channel, set
      info     = va_arg(argptr, DRIVER_INFO *);
      channel  = va_arg(argptr, INT);
      valueint = (int32_t) va_arg(argptr, int32_t); 
      status   = roc_crv_driver_set_register(info, channel, valueint);
      break;

   case CMD_GET+1000: // register, not channel, get
      info    = va_arg(argptr, DRIVER_INFO *);
      channel = va_arg(argptr, INT);
      pvalueint  = va_arg(argptr, int32_t *);
      status  = roc_crv_driver_get_register(info, channel, pvalueint);
      break;

   default:
      break;
   }

   va_end(argptr);

   return status;
}

/*------------------------------------------------------------------*/
