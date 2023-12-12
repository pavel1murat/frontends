/********************************************************************\

  Name:         frontend.c
  Created by:   Stefan Ritt

  Contents:     Experiment specific readout code (user part) of
                Midas frontend. This example simulates a "trigger
                event" and a "periodic event" which are filled with
                random data.
 
                The trigger event is filled with two banks (ADC0 and TDC0),
                both with values with a gaussian distribution between
                0 and 4096. About 100 event are produced per second.
 
                The periodic event contains one bank (PRDC) with four
                sine-wave values with a period of one minute. The
                periodic event is produced once per second and can
                be viewed in the history system.

\********************************************************************/

// P.Murat: this is a slow control frontend, which data are supposed 
// to be written into ODB

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h> // assert()

#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"
#include "proto/artdaqapp.hh"

#include "history.h"

#include "mfe.h"

#include "trk_dcs/trk_dcs_equipment.hh"

#include "trk_dcs/utils.hh"

#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTC.h"
#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTCSoftwareCFO.h"

using namespace DTCLib;
//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
const char *frontend_name       = "TRK_FE_DTC01";     // the frontend (client) name) as seen by other MIDAS clients
const char *frontend_file_name  = __FILE__;           // the frontend file name, don't change it
BOOL        frontend_call_loop  = FALSE;              // if TRUE, frontend_loop is called periodically
INT         display_period      = 3000;               // update frequency, in ms, of the frontend status page
INT         max_event_size      = 1024 * 1024;        // 1 MB : max event size produced by this frontend
INT         max_event_size_frag = 5 * 1024 * 1024;    // 5 MB : maximum event size for fragmented events (EQ_FRAGMENTED)
INT         event_buffer_size   = 10 * 1024 * 1024;   // 10 MB: // buffer size to hold events, must be > 2 * max_event_size
//-----------------------------------------------------------------------------
//              Callback routines for system transitions
//
//  These routines are called whenever a system transition like start/
//  stop of a run occurs. The routines are called on the following
//  occations:
//
//  frontend_init:  When the frontend program is started. This routine
//                  should initialize the hardware.
//
//  frontend_exit:  When the frontend program is shut down. Can be used
//                  to releas any locked resources like memory, commu-
//                  nications ports etc.
//
//  begin_of_run:   When a new run is started. Clear scalers, open
//                  rungates, etc.
//
//  end_of_run:     Called on a request to stop a run. Can send
//                  end-of-run event and close run gates.
//
//  pause_run:      When a run is paused. Should disable trigger events.
//
//  resume_run:     When a run is resumed. Should enable trigger events.
//-----------------------------------------------------------------------------
INT frontend_init      (void);
INT frontend_exit      (void);
INT begin_of_run       (INT RunNumber, char *error);
INT end_of_run         (INT RunNumber, char *error);
INT pause_run          (INT RunNumber, char *error);
INT resume_run         (INT RunNumber, char *error);
INT frontend_loop      (void);

INT read_trigger_event (char *pevent, INT off);
INT read_periodic_event(char *pevent, INT off);

INT poll_event         (INT source, INT count, BOOL test);

// INT interrupt_configure(INT cmd, INT source, POINTER_T adr);


static DTC_dtc;
//-----------------------------------------------------------------------------
// P.M. use argc and argv via mfc provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int    argc;
  char** argv;

  mfe_get_args(&argc,&argv);
  
  if ((argc == 3) and (strcmp(argv[1],"-c") == 0)) {
//-----------------------------------------------------------------------------
// assume FCL file : 'tfm_frontend -c xxx.fcl'
//-----------------------------------------------------------------------------
    std::string fcl_fn = argv[2];

    fhicl::ParameterSet top_ps = LoadParameterSet(fcl_fn);
    fhicl::ParameterSet tfm_ps = top_ps.get<fhicl::ParameterSet>("tfm_frontend",fhicl::ParameterSet());

    // _useRunInfoDB = tfm_ps.get<bool>("useRunInfoDB",false);
  }
//-----------------------------------------------------------------------------
// this is for later (may be)
//-----------------------------------------------------------------------------
  // std::unique_ptr<artdaq::Commandable> comm =  std::make_unique<artdaq::Commandable>();
  // _commander = artdaq::MakeCommanderPlugin(ps, *comm);
  // _commander->run_server();

  set_equipment_status(equipment[0].name, "Initialising...", "yellow");

  return SUCCESS;
  // return FE_ERR_HW;
}

//-----------------------------------------------------------------------------
INT frontend_exit() {
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// put here clear scalers etc.
//-----------------------------------------------------------------------------
INT begin_of_run(INT RunNumber, char *error) {
  printf("%s::%s 003: done starting, run=%6i rc=%i\n",frontend_name,__func__,RunNumber,rc);
  // change frontend status color green
  db_set_value(hDB, 0, color_str.c_str(), "greenLight", 32, 1, TID_STRING);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT end_of_run(INT RunNumber, char *Error) {
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT pause_run(INT RunNumber, char *error) {
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT resume_run(INT RunNumber, char *error) {
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT frontend_loop() {
   /* if frontend_call_loop is true, this routine gets called when
      the frontend is idle or once between every event */
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE  : break;
  case CMD_INTERRUPT_DISABLE : break;
  case CMD_INTERRUPT_ATTACH  : break;
  case CMD_INTERRUPT_DETACH  : break;
  }
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// Readout routines for different event types
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// trigger events
// Polling routine for events. Returns TRUE if event is available. 
// If test equals TRUE, don't return. The test flag is used to time the polling
//-----------------------------------------------------------------------------
INT poll_event(INT source, INT count, BOOL test) {
  DWORD flag;

  /* poll hardware and set flag to TRUE if new event is available */
  for (int i = 0; i < count; i++) {
    flag = TRUE;
    
    if (flag) {
      if (!test) {
        return TRUE;
      }
    }
  }
  
  return 0;
}

//-----------------------------------------------------------------------------
void parse_spi_data(uint16_t* dat, int nw) {

  const char* keys[] = {
    "I3.3","I2.5","I1.8HV","IHV5.0","VDMBHV5.0","V1.8HV","V3.3HV" ,"V2.5"    , 
    "A0"  ,"A1"  ,"A2"    ,"A3"    ,"I1.8CAL"  ,"I1.2"  ,"ICAL5.0","ADCSPARE",
    "V3.3","VCAL5.0","V1.8CAL","V1.0","ROCPCBTEMP","HVPCBTEMP","CALPCBTEMP","RTD",
    "ROC_RAIL_1V(mV)","ROC_RAIL_1.8V(mV)","ROC_RAIL_2.5V(mV)","ROC_TEMP(CELSIUS)",
    "CAL_RAIL_1V(mV)","CAL_RAIL_1.8V(mV)","CAL_RAIL_2.5V(mV)","CAL_TEMP(CELSIUS)",
    "HV_RAIL_1V(mV)","HV_RAIL_1.8V(mV)","HV_RAIL_2.5V(mV)","HV_TEMP(CELSIUS)"
  };
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

  float val[nw];

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

    printf("%-20s : %10.3f\n",keys[i],val[i]);
  }
}

//-----------------------------------------------------------------------------
// This function gets called periodically by the MFE framework.
// the period is set in the EQUIPMENT structs at the top of the file).
//
// read SPI data from ROC, in 16-bit words
// decoding is done in parse_spi_data
//-----------------------------------------------------------------------------
INT read_periodic_event(char *pevent, INT off) {
  int      rc(0);

  int nw     = TrkSpiData_t::nWords();
  int nb_spi = nw*2;

  try {
    _dtc->WriteROCRegister(DTC_Link_0,258,0x0000  ,false,100);

    _dtc->ReadROCRegister (DTC_Link_0,roc_address_t(128),100); // printf("0x%04x\n",u);
    _dtc->ReadROCRegister (DTC_Link_0,roc_address_t(129),100); // printf("0x%04x\n",u);

    std::vector<uint16_t> v;
    _dtc->ReadROCBlock(v,DTC_Link_0,258,nw,false,100);
//-----------------------------------------------------------------------------
// convert to voltages/temperatures
//-----------------------------------------------------------------------------
    
//-----------------------------------------------------------------------------
// at this point copy data into a MIDAS bank
//-----------------------------------------------------------------------------
    char bk_name[100];
    sprintf(bk_name,"DTC0");

    struct bank_t {
      DWORD  nw;
      DWORD  data[]
    }; 

    bank_t* bank_data;
    bk_create(pevent, bk_name, TID_FLOAT, &bank_data);
    memcpy( hdata, GPUDATA->gpu_data_his[itq], GPUDATA->gpu_data_his_size[itq]); // data array of sixe given by first word of the data bank
    hdata += GPUDATA->gpu_data_his_size[itq] / sizeof(hdata[0]);
    bk_close(pevent, hdata);
  }
  catch (...) {
//-----------------------------------------------------------------------------
// need to know which kind of exception could be thrown
//-----------------------------------------------------------------------------
    printf("ERROR in %s::%s : readSPI readout error\n");
    return -1;
  }
  

  return 0; // event size 
}
