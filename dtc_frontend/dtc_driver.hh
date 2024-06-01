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

#include "otsdaq-mu2e-tracker/ui/DtcInterface.hh"

typedef struct {
  int                   pcieAddr;
  trkdaq::DtcInterface* dtc_i;
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
// DTC temperature and voltages for slow monitoring
//-----------------------------------------------------------------------------
struct DtcData_t {
  float Temp;        // 0x9010: FPGA temperature    : T(C) = val(0x9010)*503.975/4096 - 273.15
  float VCCINT;      // 0x9014: FPGA VCCINT voltage : V(V) = (ADC code)/4095*3.
  float VCCAUX;      // 0x9018: FPGA VCCAUX voltage : V(V) = (ADC code)/4095*3.
  float VCBRAM;      // 0x901c: FPGA VCBRAM voltage : V(V) = (ADC code)/4095*3.
};

//-----------------------------------------------------------------------------
// internally, a MIDAS driver stored only floats
// reserve 4 words per DTC for a temp and three voltages
//-----------------------------------------------------------------------------
int const DTC_NREG_HIST     =  4;       // number of registers to monitor as a part of history

const int DTC_REG_HIST[DTC_NREG_HIST] = {
  0x9010, 0x9014, 0x9018, 0x901c              // temperature, voltages
};

int const DTC_NREG_NON_HIST = 15+2*6;     // number of DTC registers to monitor (but not save to history)

const int DTC_REG_NON_HIST[DTC_NREG_NON_HIST] = {
  0x9004, 0x9100, 0x9114, 0x9140, 0x9144, 0x9158, 0x9188, 0x91a8, 0x91ac, 0x91bc, 
  0x91c0, 0x91c4, 0x91f4, 0x91f8, 0x93e0,

  0x9630, 0x9650,                  // Link 0: N(DTC requests) and N(heartbeats) 
  0x9634, 0x9654,                  // Link 1: N(DTC requests) and N(heartbeats) 
  0x9638, 0x9658,                  // Link 2: N(DTC requests) and N(heartbeats) 
  0x963c, 0x965c,                  // Link 3: N(DTC requests) and N(heartbeats) 
  0x9640, 0x9660,                  // Link 4: N(DTC requests) and N(heartbeats) 
  0x9644, 0x9664                   // Link 5: N(DTC requests) and N(heartbeats) 
};

