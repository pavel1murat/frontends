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

#include "frontends/roc_frontend/roc_driver.hh"

using namespace DTCLib; 

int         _link(1);
DTC_Link_ID _roc          = DTC_Link_ID(_link);  // ROC =1
int         _dtcID(1);  

int const   _sleepTimeDTC     ( 200);
int const   _sleepTimeROC     (2500);
int const   _sleepTimeROCReset(4000);

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

  delete _dtc;
  
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
void monica_digi_clear(DTCLib::DTC* Dtc, int Link) {
//-----------------------------------------------------------------------------
//  Monica's digi_clear
//  this will proceed in 3 steps each for HV and CAL DIGIs:
// 1) pass TWI address and data toTWI controller (fiber is enabled by default)
// 2) write TWI INIT high
// 3) write TWI INIT low
//-----------------------------------------------------------------------------
  auto link = DTC_Link_ID(Link);

  Dtc->WriteROCRegister(link,28,0x10,false,1000); // 

  // Writing 0 & 1 to  address=16 for HV DIGIs ??? 

  Dtc->WriteROCRegister(link,27,0x00,false,1000); // write 0 
  Dtc->WriteROCRegister(link,26,0x01,false,1000); // toggle INIT 
  Dtc->WriteROCRegister(link,26,0x00,false,1000); // 

  Dtc->WriteROCRegister(link,27,0x01,false,1000); //  write 1 
  Dtc->WriteROCRegister(link,26,0x01,false,1000); //  toggle INIT
  Dtc->WriteROCRegister(link,26,0x00,false,1000); // 

  // echo "Writing 0 & 1 to  address=16 for CAL DIGIs"
  Dtc->WriteROCRegister(link,25,0x10,false,1000); // 

  Dtc->WriteROCRegister(link,24,0x00,false,1000); // write 0
  Dtc->WriteROCRegister(link,23,0x01,false,1000); // toggle INIT
  Dtc->WriteROCRegister(link,23,0x00,false,1000); // 

  Dtc->WriteROCRegister(link,24,0x01,false,1000); // write 1
  Dtc->WriteROCRegister(link,23,0x01,false,1000); // toggle INIT
  Dtc->WriteROCRegister(link,23,0x00,false,1000); // 
}

//-----------------------------------------------------------------------------
void monica_var_link_config(DTCLib::DTC* Dtc, int Link) {

  mu2edev* dev = Dtc->GetDevice();

  dev->write_register(0x91a8,100,0);                // disable event window marker - set deltaT = 0
  std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeDTC));

  auto link = DTC_Link_ID(Link);
  Dtc->WriteROCRegister(link,14,     1,false,1000); // reset ROC
  std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeROCReset));

  Dtc->WriteROCRegister(link, 8,0x030f,false,1000); // configure ROC to read all 4 lanes
  std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeROC));

  // added register for selecting kind of data to report in DTC status bits
  // Use with pattern data. Set to zero, ie STATUS=0x55, when taking DIGI data 
  // rocUtil -a 30  -w 0  -l $LINK write_register > /dev/null

  Dtc->WriteROCRegister(link,30,0x0000,false,1000);
  std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeROC));

  // echo "Setting packet format version to 1"
  // rocUtil -a 29  -w 1  -l $LINK write_register > /dev/null

  Dtc->WriteROCRegister(link,29,0x0001,false,1000);
  std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeROC));
}


//-----------------------------------------------------------------------------
void parse_spi_data(uint16_t* dat, int nw, float* val) {
  // const char* keys[] = {
  //   "I3.3"           ,"I2.5"             ,"I1.8HV"           ,"IHV5.0"           , //  0:ROC_000
  //   "VDMBHV5.0"      ,"V1.8HV"           ,"V3.3HV"           ,"V2.5"             , //  4:ROC_001
  //   "A0"             ,"A1"               ,"A2"               ,"A3"               , //  8:
  //   "I1.8CAL"        ,"I1.2"             ,"ICAL5.0"          ,"ADCSPARE"         , // 12:ROC_003
  //   "V3.3"           ,"VCAL5.0"          ,"V1.8CAL"          ,"V1.0"             , // 16:ROC_004
  //   "ROCPCBTEMP"     ,"HVPCBTEMP"        ,"CALPCBTEMP"       ,"RTD"              , // 20:ROC_005
  //   "ROC_RAIL_1V(mV)","ROC_RAIL_1.8V(mV)","ROC_RAIL_2.5V(mV)","ROC_TEMP(CELSIUS)", // 24:ROC_006
  //   "CAL_RAIL_1V(mV)","CAL_RAIL_1.8V(mV)","CAL_RAIL_2.5V(mV)","CAL_TEMP(CELSIUS)", // 28:ROC_007
  //   "HV_RAIL_1V(mV)" ,"HV_RAIL_1.8V(mV)" ,"HV_RAIL_2.5V(mV)" ,"HV_TEMP(CELSIUS)"   // 32:ROC_008
  // };
//-----------------------------------------------------------------------------
// primary source : https://github.com/bonventre/trackerScripts/blob/master/constants.py#L99
//-----------------------------------------------------------------------------
  struct constants_t {
    float iconst  = 3.3 /(4096*0.006*20);
    float iconst5 = 3.25/(4096*0.500*20);
    float iconst1 = 3.25/(4096*0.005*20);
    float toffset = 0.509;
    float tslope  = 0.00645;
    float tconst  = 0.000806;
    float tlm45   = 0.080566;  
  } constants;

  for (int i=0; i<nw; i++) {
    if (i==20 or i==21 or i==22) {
      val[i] = dat[i]*constants.tlm45;
    }
    else if (i==0 or i==1 or i==2 or i==12 or i==13) {
      val[i] = dat[i]*constants.iconst;
    }
    else if (i==3 or i==14) {
      val[i] = dat[i]*constants.iconst5 ;
    }
    else if (i==4 or i==5 or i==6 or i==7 or i==16 or i==17 or i==18 or i==19) {
      val[i] = dat[i]*3.3*2/4096 ; 
    }
    else if (i==15) {
      val[i] = dat[i]*3.3/4096;
    }
    else if (i==23) {
      val[i] = dat[i]*3.3/4096;
    }
    else if (i==8 or i==9 or i==10 or i==11) {
      val[i] = dat[i];
    }
    else if (i > 23) {
      if   ((i%4) < 3) val[i] = dat[i]/8.;
      else             val[i] = dat[i]/16.-273.15;
    }

    // printf("%-20s : %10.3f\n",keys[i],val[i]);
  }
}

//-----------------------------------------------------------------------------
// this is the function which reads the registers
// ----------------------------------------------
// ROC: all parameters come as one block read, assume that always real all of them
//-----------------------------------------------------------------------------
INT driver_get(DRIVER_INFO* Info, INT Channel, float *Pvalue) {
  //    const int reg [4] = {0x9010, 0x9014, 0x9018, 0x901c}; 
  // int rc;
  // uint32_t val(0);
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
   // char str[80];
   // sprintf(str, "GET %d", channel);
   // BD_PUTS(str);
   // BD_GETS(str, sizeof(str), ">", DEFAULT_TIMEOUT);
//-----------------------------------------------------------------------------
// read when channel = 0
// probably a frontend per ROC and read multithreaded
//-----------------------------------------------------------------------------
   if (Channel == 0) {
     monica_var_link_config(_dtc,_roc);
     monica_digi_clear     (_dtc,_roc);
//-----------------------------------------------------------------------------
// after writing into reg 258, sleep for some time, 
// then wait till reg 128 returns non-zero
//-----------------------------------------------------------------------------
    _dtc->WriteROCRegister   (_roc,258,0x0000,false,100);
    std::this_thread::sleep_for(std::chrono::microseconds(_sleepTimeROC));

    uint16_t u; 
    while ((u = _dtc->ReadROCRegister(_roc,128,100)) == 0) {}; 
//-----------------------------------------------------------------------------
// register 129: number of words to read, currently-  (+ 4) (ask Monica)
//-----------------------------------------------------------------------------
    int nw = _dtc->ReadROCRegister(_roc,129,100); // printf("reg:%03i val:0x%04x\n",129,nw);

    nw = nw-4;
    std::vector<uint16_t> spi;
    _dtc->ReadROCBlock(spi,_roc,258,nw,false,100);
    parse_spi_data(&spi[0],nw,Info->array);
   }
//-----------------------------------------------------------------------------
// channel=0: temperature, the rest - voltages
//-----------------------------------------------------------------------------
   if      (Channel  < 36) *Pvalue = Info->array[Channel];
   else {
     printf("driver_get ROC_DRIVER: channel = %i. IN TROUBLE\n",Channel);
   }
   // printf("channel: %i val: %i pvalue[channel] = %10.3f\n",channel,val,*pvalue);
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
