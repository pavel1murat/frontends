/********************************************************************\

\********************************************************************/

#undef NDEBUG // midas required assert() to be always enabled

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h> // assert()

#include "midas.h"
#include "history.h"
// #include "experim.h"

#include "mfe.h"
// #include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
// #include "proto/artdaqapp.hh"
#include "artdaq/Application/LoadParameterSet.hh"

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tfm/db_runinfo.hh"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "feslow";
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
INT begin_of_run (INT RunNumber, char *error);
INT end_of_run   (INT RunNumber, char *error);
INT pause_run    (INT RunNumber, char *error);
INT resume_run   (INT RunNumber, char *error);
INT frontend_loop(void);

INT read_trigger_event(char *pevent, INT off);
INT read_slow_event   (char *pevent, INT off);

INT poll_event(INT source, INT count, BOOL test);

// INT interrupt_configure(INT cmd, INT source, POINTER_T adr);
#define EQ_EVID 1

//-----------------------------------------------------------------------------
BOOL equipment_common_overwrite = TRUE;

EQUIPMENT equipment[] = {
  {
    "feslow",                       // eq name
    EQUIPMENT_INFO {
      EQ_EVID, (1<<EQ_EVID), /* event ID, trigger mask */
      "SYSTEM",              /* event buffer */
      EQ_PERIODIC,          /* equipment type */
      0,                    /* event source */
      "MIDAS",              /* format */
      TRUE,                 /* enabled */
      RO_RUNNING|RO_STOPPED|RO_PAUSED|RO_ODB, /* When to read */
      1000,                 /* poll every so milliseconds */
      0,                    /* stop run after this event limit */
      0,                    /* number of sub events */
      1,                    /* history period, seconds*/
      "", "", "",
//      2, 0,                               // event ID, trigger mask
//      "SYSTEM",                           // event buffer name
//      EQ_PERIODIC,                        // one of EQ_xx (equipment type)
//      0,                                  // event source (LAM/IRQ)
//      "MIDAS",                            // data format to produce
//      TRUE,                               // enable flag value
//                                          // combination of Read-On flags R)_xxx - read when running and on transitions 
//                                          // and on ODB updates
//      RO_RUNNING | RO_TRANSITIONS | RO_ODB,
//      1000,                               // readout interval/polling time in ms (1 sec)
//      0,                                  // stop run after this event limit - probably, 0=never
//      0,                                  // number of sub events
//      10,                                 // log history every ten seconds
//      "",                                 // host on which frontend is running
//      "",                                 // frontend name
//      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    read_slow_event,                      // pointer to user readout routine
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
      "dcs_prototype",                    // frontend name
      "",                                 // source file ised for user FE
      "",                                 // current status of equipment
      "",                                 // color or class used by mhttpd for status
      0,                                  // hidden flag
      0                                   // event buffer write cache size
    },
    nullptr,                              // pointer to user readout routine
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
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
int count_slow = 0;
static int configure() {
  // int size, status;

   //size = sizeof(int);
   //status = db_get_value(hDB, 0, "/Equipment/" EQ_NAME "/Settings/event_size", &event_size, &size, TID_INT, TRUE);
   //printf("Event size set to %d bytes\n", event_size);
   
   return SUCCESS;
}

TAG temp_tag[] = {
   {"temps", TID_DOUBLE, 3},
};

INT frontend_init() {
  int    argc;
  char** argv;

  mfe_get_args(&argc,&argv);
  
  if ((argc == 3) and (strcmp(argv[1],"-c") == 0)) {
//-----------------------------------------------------------------------------
// assume FCL file : 'tfm_frontend -c xxx.fcl'
//-----------------------------------------------------------------------------
    // std::string fcl_fn = argv[2];

    // fhicl::ParameterSet top_ps = LoadParameterSet(fcl_fn);
    // fhicl::ParameterSet tfm_ps = top_ps.get<fhicl::ParameterSet>("tfm_frontend",fhicl::ParameterSet());

    // _useRunInfoDB = tfm_ps.get<bool>("useRunInfoDB",false);
  }
//-----------------------------------------------------------------------------
// this is for later - when we learn how to communicate with the TF manager
// properly
//-----------------------------------------------------------------------------
  // std::unique_ptr<artdaq::Commandable> comm =  std::make_unique<artdaq::Commandable>();
  // _commander = artdaq::MakeCommanderPlugin(ps, *comm);
  // _commander->run_server();
   configure();

   // [TODO P.Murat] hs_define_event(1,"Temperature", 1,temp_tag);

  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT frontend_exit() {
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// put here clear scalers etc.
//-----------------------------------------------------------------------------
INT begin_of_run(INT RunNumber, char *error) {
  printf("Begin run %d\n", RunNumber);
  //  gbl_run_number = run_number;

  configure();

  count_slow = 0;


  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT end_of_run(INT RunNumber, char *Error) {
  printf("End run %d!\n", RunNumber);

  cm_msg(MINFO, frontend_name, "read %d slow events", count_slow);
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
  if (test) ss_sleep (count);
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
double get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}


INT read_slow_event(char *pevent, INT off) {
  //  UINT32 *pdata;

  bk_init32(pevent);

  count_slow++;

  double* pdatad;
  bk_create(pevent, "SLOW", TID_DOUBLE, (void**) &pdatad);

  time_t t = time(NULL);
  pdatad[0] = count_slow;
  pdatad[1] = t;
  pdatad[2] = 100.0*sin(M_PI*t/60);
  printf("time %d, data %f\n", (int)t, pdatad[2]);

  bk_close(pevent, pdatad + 3);

  // [TODO P.Murat] hs_write_event(1, pdatad, sizeof(3*8));

  return bk_size(pevent);
}
