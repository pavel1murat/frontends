/********************************************************************\

  Name:         nulldev.h
  Created by:   Stefan Ritt

  Contents:     Device driver function declarations for NULL device

  $Id:$

\********************************************************************/
/* Store any parameters the device driver needs in following 
   structure. Edit the DTC_DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

#include "dtcInterfaceLib/DTC.h"

typedef struct {
  int          address;
  int          dtcID;
  DTCLib::DTC* dtc;
} DTC_DRIVER_SETTINGS;

/* the following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  DTC_DRIVER_SETTINGS driver_settings;
  float               *array;
  INT                 num_channels;
  INT                 (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void                *bd_info;                /* private info of bus driver  */
  HNDLE               hkey;                    /* ODB key for bus driver info */
} DTC_DRIVER_INFO;


INT dtc_driver(INT cmd, ...);
//-----------------------------------------------------------------------------
// ARTDAQ metrics reported by the data receiver, assume everything is 4 bytes
//-----------------------------------------------------------------------------
struct DtcData_t {
  float Temp;        // 0x9010: FPGA temperature    : T(C) = val(0x9010)*503.975/4096 - 273.15
  float VCCINT;      // 0x9014: FPGA VCCINT voltage : V(V) = (ADC code)/4095*3.
  float VCCAUX;      // 0x9018: FPGA VCCAUX voltage : V(V) = (ADC code)/4095*3.
  float VCBRAM;      // 0x901c: FPGA VCBRAM voltage : V(V) = (ADC code)/4095*3.
  int   r_0x9004;    // 0x9004: DTC version 
  int   r_0x9100;    // 0x9100: DTC control register
  int   r_0x9140;    // 0x9140: fibers locked
};
//-----------------------------------------------------------------------------
// internally, a MIDAS driver stored only floats
// reserve seven (7) words just for the first attempt to display
//-----------------------------------------------------------------------------
int const DTC_DRIVER_NWORDS =  7;

