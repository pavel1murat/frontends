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

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h> // assert()

#include "midas.h"
#include "experim.h"

#include "mfe.h"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"
#include "proto/artdaqapp.hh"

#include "tfm_frontend/my_xmlrpc.hh"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "tfm_frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

/* a frontend status page is displayed with this frequency in ms */
INT display_period = 3000;

/* maximum event size produced by this frontend */
INT max_event_size = 1024 * 1024; // 1 MB

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024; // 5 MB

/* buffer size to hold events */
INT event_buffer_size = 10 * 1024 * 1024; // 10 MB, must be > 2 * max_event_size

/*-- Function declarations -----------------------------------------*/

INT frontend_init(void);
INT frontend_exit(void);
INT begin_of_run (INT run_number, char *error);
INT end_of_run   (INT run_number, char *error);
INT pause_run    (INT run_number, char *error);
INT resume_run   (INT run_number, char *error);
INT frontend_loop(void);

INT read_trigger_event (char *pevent, INT off);
INT read_periodic_event(char *pevent, INT off);

INT poll_event(INT source, INT count, BOOL test);

// INT interrupt_configure(INT cmd, INT source, POINTER_T adr);

std::unique_ptr<artdaq::CommanderInterface>  _commander;
int my_xmlrpc(int argc, const char** argv);

/*-- Equipment list ------------------------------------------------*/

BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {
    "trigger_farm",                       // eq name
    EQUIPMENT_INFO {
      2, 0,                               // event ID, trigger mask
      "SYSTEM",                           // event buffer name
      EQ_PERIODIC,                        // one of EQ_xx (equipment type)
      0,                                  // event source (LAM/IRQ)
      "MIDAS",                            // data format to produce
      TRUE,                               // enable flag value
                                          // combination of Read-On flags R)_xxx - read when running and on transitions 
                                          // and on ODB updates
      RO_RUNNING | RO_TRANSITIONS | RO_ODB,
      1000,                               // readout interval/polling time in ms (1 sec)
      0,                                  // stop run after this event limit - probably, 0=never
      0,                                  // number of sub events
      10,                                 // log history every ten seconds
      "",                                 // host on which frontend is running
      "",                                 // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    read_periodic_event,                  // pointer to user readout routine
    nullptr,                              // class driver routine
    nullptr,                              // device driver list
    nullptr,                              // init string for fixed events or bank list
    nullptr,                              // private data for class driver
    0,                                    // one of FE_xxx
    DWORD(0),                             // last time event was read
    DWORD(0),                             // last time idle func was called
    DWORD(0),                             // poll period
    0,                                    // FORMAT_xxx
    HNDLE(-1),                            // MIDAS buffer handle (int)
    HNDLE(-1),                           // handle to variable subtree in ODB
    DWORD(0),                            // serial number
    DWORD(0),                            // subevent number
    DWORD(0),                            // updates FE->ODB
    DWORD(0),                            // updates ODB-->FE
    DWORD(0),                            // bytes sent
    DWORD(0),                            // events sent
    DWORD(0),                            // events collected
    EQUIPMENT_STATS{
      0,                                 // events sent
      0,                                 // events_per_sec
      0                                  // kbytes_per_sec
    }
  },
//-----------------------------------------------------------------------------
// an empty frontend structure marks end of the equipment list
//-----------------------------------------------------------------------------
  {"",                                   // empty structure
   EQUIPMENT_INFO {
      2, 0,                               // event ID, trigger mask
      "SYSTEM",                           // event buffer name
      EQ_PERIODIC,                        // one of EQ_xx (equipment type)
      0,                                  // event source (LAM/IRQ)
      "MIDAS",                            // data format to produce
      TRUE,                               // enable flag value
      // combination of Read-On flags R)_xxx - read when running and on transitions 
      // and on ODB updates
      RO_RUNNING | RO_TRANSITIONS | RO_ODB,
      1000,                               // readout interval/polling time in ms (1 sec)
      0,                                  // stop run after this event limit - probably, 0=never
      0,                                  // number of sub events
      10,                                 // log history every ten seconds
      "",                                 // host on which frontend is running
      "",                                 // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    read_periodic_event,                  // pointer to user readout routine
    nullptr,                              // class driver routine
    nullptr,                              // device driver list
    nullptr,                              // init string for fixed events or bank list
    nullptr,                              // private data for class driver
    0,                                    // one of FE_xxx
    DWORD(0),                             // last time event was read
    DWORD(0),                             // last time idle func was called
    DWORD(0),                             // poll period
    0,                                    // FORMAT_xxx
    HNDLE(-1),                            // MIDAS buffer handle (int)
    HNDLE(-1),                           // handle to variable subtree in ODB
    DWORD(0),                            // serial number
    DWORD(0),                            // subevent number
    DWORD(0),                            // updates FE->ODB
    DWORD(0),                            // updates ODB-->FE
    DWORD(0),                            // bytes sent
    DWORD(0),                            // events sent
    DWORD(0),                            // events collected
    EQUIPMENT_STATS{
      0,                                 // events sent
      0,                                 // events_per_sec
      0                                  // kbytes_per_sec
    }
  }
};

/********************************************************************\
              Callback routines for system transitions

  These routines are called whenever a system transition like start/
  stop of a run occurs. The routines are called on the following
  occations:

  frontend_init:  When the frontend program is started. This routine
                  should initialize the hardware.

  frontend_exit:  When the frontend program is shut down. Can be used
                  to releas any locked resources like memory, commu-
                  nications ports etc.

  begin_of_run:   When a new run is started. Clear scalers, open
                  rungates, etc.

  end_of_run:     Called on a request to stop a run. Can send
                  end-of-run event and close run gates.

  pause_run:      When a run is paused. Should disable trigger events.

  resume_run:     When a run is resumed. Should enable trigger events.
\********************************************************************/

//-----------------------------------------------------------------------------
// the farm should be started independent on the frontend (or not ?)
// print message and return FE_ERR_HW if frontend should not be started 
//-----------------------------------------------------------------------------
INT frontend_init() {
  // std::string fcl_fn("farm_frontend.fcl");

	// fhicl::ParameterSet ps = LoadParameterSet(fcl_fn);

  // std::unique_ptr<artdaq::Commandable> comm =  std::make_unique<artdaq::Commandable>();

  // _commander = artdaq::MakeCommanderPlugin(ps, *comm);
  // _commander->run_server();

  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT frontend_exit() {
   return SUCCESS;
}

//-----------------------------------------------------------------------------
// put here clear scalers etc.
//-----------------------------------------------------------------------------
INT begin_of_run(INT run_number, char *error) {

  int nw(6);

  std::string exec_name = "none";
 
  std::string       w    [100];
  const char*       words[100];

  words[0]   = exec_name.data();  // executable name, not used

  w[0] = "http://localhost:18000/RPC2";
  w[1] = "state_change";
  w[2] = "daqint";
//-----------------------------------------------------------------------------
// first 'configure'
//-----------------------------------------------------------------------------
  w[3] = "configuring"; 

  printf("tfm_frontend::%s 000: trying to configure\n",__func__);

  // char tmp[100];
  // //  sprintf(tmp,"\'struct{run_number:i/%i}\'",run_number);
  // sprintf(tmp,"struct{run_number:i/%i}",run_number);
  // w[4] = tmp;
  w[4]  = "struct/{run_number:i/"+std::to_string(run_number);
  w[4] += "}";
  
  words[1] = w[0].data();
  words[2] = w[1].data();
  words[3] = w[2].data();
  words[4] = w[3].data();
  words[5] = w[4].data();
  
  printf("tfm_frontend::%s 001: trying to configure, nw = %i\n",__func__,nw);
  for (int i=0; i<nw; i++) {
    printf("tfm_frontend::%s 001: trying to configure, words[%i] = %s\n",__func__,i,words[i]);
  }

  std::string res;
  my_xmlrpc(nw, words, res);

  printf("tfm_frontend::%s 0011: after my_xmlrpc  command: result:%s\n",__func__,res.data());
//-----------------------------------------------------------------------------
// now wait till completion
//-----------------------------------------------------------------------------
  int completed = 0;

  std::string token;

  w[1] = "get_state";
  w[2] = "daqint";

  words[1] = w[0].data();
  words[2] = w[1].data();
  words[3] = w[2].data();
  nw       = 4;

  while ((token != "configured") or (completed != 100)) {

    my_xmlrpc(nw, words, res);
    sleep(1);

    size_t pos = res.find(":");
    token = res.substr(0, pos);
    res.erase(0, pos + 1);

    completed = std::stoi(res);

    printf(" --- waiting: token:%s  completed: %i\n",token.data(),completed);
  }

  printf("tfm_frontend::%s 0011: done to configure\n",__func__);
//-----------------------------------------------------------------------------
// how do I know that the configure step suceeded ?
// have to wait and make sure ? wait for a message from the farm manager
//
// // assuming it was OK, 'start'
//-----------------------------------------------------------------------------
  w[1] = "state_change";
  w[2] = "daqint";
  w[3] = "starting"; 
  w[4] = "struct/{ignored_variable:i/999}";

  words[1] = w[0].data();
  words[2] = w[1].data();
  words[3] = w[2].data();
  words[4] = w[3].data();
  words[5] = w[4].data();
  nw = 6;
//-----------------------------------------------------------------------------
// send 'start' command
//-----------------------------------------------------------------------------
  printf("tfm_frontend::%s 002: trying to START\n",__func__);
  for (int i=0; i<nw; i++) {
    printf("tfm_frontend::%s 002: trying to START, words[%i] = %s\n",__func__,i,words[i]);
  }

  my_xmlrpc(nw, words,res);

  printf("tfm_frontend::%s 003: done, exiting\n",__func__);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT end_of_run(INT run_number, char *error) {
   return SUCCESS;
}

//-----------------------------------------------------------------------------
INT pause_run(INT run_number, char *error) {
   return SUCCESS;
}

//-----------------------------------------------------------------------------
INT resume_run(INT run_number, char *error) {
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

/*-- Event readout -------------------------------------------------*/
// This function gets called whenever poll_event() returns TRUE (the
// MFE framework calls poll_event() regularly).

//-----------------------------------------------------------------------------
// Periodic event
// This function gets called periodically by the MFE framework (the
// period is set in the EQUIPMENT structs at the top of the file).
//-----------------------------------------------------------------------------
INT read_periodic_event(char *pevent, INT off) {
  UINT32 *pdata;

  /* init bank structure */
  bk_init(pevent);

  /* create a bank called PRDC */
  bk_create(pevent, "PRDC", TID_UINT32, (void **)&pdata);

  /* following code "simulates" some values in sine wave form */
  for (int i = 0; i < 16; i++)
    *pdata++ = 100*sin(M_PI*time(NULL)/60+i/2.0)+100;

  bk_close(pevent, pdata);

  return bk_size(pevent);
}


// #ifdef STANDALONE
int main(int ardc, char** argc) {

  char error[100];
  begin_of_run(10, error);
}
// #endif
