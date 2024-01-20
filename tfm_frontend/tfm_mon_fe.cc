///////////////////////////////////////////////////////////////////////////////
// trigger farm monitoring frontend
///////////////////////////////////////////////////////////////////////////////
#undef NDEBUG // midas required assert() to be always enabled

#include "TRACE/tracemf.h"
#define  TRACE_NAME "tfm_mon_fe"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h> // assert()

#include "midas.h"
// #include "experim.h"

#include "mfe.h"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"
#include "proto/artdaqapp.hh"

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tfm_frontend/tfm_mon_fe.hh"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name      = TRACE_NAME;
const char *frontend_file_name = __FILE__  ; // The frontend file name, don't change
BOOL        frontend_call_loop = TRUE      ; // frontend_loop is called periodically if TRUE
INT         display_period     = 3000      ; // display freq in ms for the frontend status page

/* maximum event size produced by this frontend */
// INT max_event_size = 1024 * 1024; // 1 MB
INT max_event_size             = 1000; // 1 MB

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 5 * 1024 * 1024; // 5 MB

/* buffer size to hold events */
INT event_buffer_size = 10 * max_event_size; // 10 MB, must be > 2 * max_event_size

/*-- Function declarations -----------------------------------------*/

INT frontend_init(void);
INT frontend_exit(void);
INT begin_of_run (INT RunNumber, char *error);
INT end_of_run   (INT RunNumber, char *error);
INT pause_run    (INT RunNumber, char *error);
INT resume_run   (INT RunNumber, char *error);
INT frontend_loop(void);

INT read_trigger_event (char *pevent, INT off);
INT read_periodic_event(char *pevent, INT off);

INT poll_event(INT source, INT count, BOOL test);

std::unique_ptr<artdaq::CommanderInterface>  _commander;

// TODO : need to get the port out of the ODB (or from FCL)
// for now - fix manually

std::string _xmlrpcUrl = "http://localhost:15000/RPC2";
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

static xmlrpc_env     _env;
//-----------------------------------------------------------------------------
// the farm should be started independent on the frontend (or not ?)
// print message and return FE_ERR_HW if frontend should not be started 
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int         argc;
  char**      argv;
  HNDLE       hDB, hKey;
  char        active_conf[100];

  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);
  int   sz = sizeof(active_conf);
  db_get_value(hDB, 0, "/Experiment/ActiveConfiguration", &active_conf, &sz, TID_STRING, TRUE);

  char key[200];
  sprintf(key,"/Experiment/RunConfigurations/%s",active_conf);
	db_find_key(hDB, 0, key, &hKey);
//-----------------------------------------------------------------------------
// ARTDAQ_PARTITION_NUMBER also comes from the active configuration ODB
//-----------------------------------------------------------------------------
  int partition;
  sz = sizeof(int);
  db_get_value(hDB, hKey, "ARTDAQ_PARTITION_NUMBER", &partition, &sz, TID_INT32, TRUE);
  int port_number = 10000+1000*partition;

  char url[100];
  sprintf(url,"http://localhost:%i/RPC2",port_number);
  _xmlrpcUrl = url;

  TLOG(TLVL_DEBUG+4) << "farm_manager _xmlrpcUrl   :" << _xmlrpcUrl ;
//-----------------------------------------------------------------------------
// this is for later - when we learn how to communicate with the TF manager
// properly
//
// init XML RPC
//-----------------------------------------------------------------------------
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "tfm_mon_fe", "v1_0");
  xmlrpc_env_init(&_env);

  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT frontend_exit() {
  xmlrpc_env_clean(&_env);
  xmlrpc_client_cleanup();

  return SUCCESS;
}


//-----------------------------------------------------------------------------
// put here clear scalers etc.
//-----------------------------------------------------------------------------
INT begin_of_run(INT RunNumber, char *error) {
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
  ss_sleep(10);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// Readout routines for different event types
//-----------------------------------------------------------------------------
/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  return 1;
};


//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
};


/*-- Event readout -------------------------------------------------*/
// This function gets called whenever poll_event() returns TRUE (the
// MFE framework calls poll_event() regularly).

//-----------------------------------------------------------------------------
// Periodic event
// This function gets called periodically by the MFE framework (the
// period is set in the EQUIPMENT structs at the top of the file).
//-----------------------------------------------------------------------------
// INT read_periodic_event(char *pevent, INT off) {
//   UINT32 *pdata;

//   /* init bank structure */
//   bk_init(pevent);

//   /* create a bank called PRDC */
//   bk_create(pevent, "FARM", TID_UINT32, (void **)&pdata);

//   /* following code "simulates" some values in sine wave form */
//   for (int i = 0; i < 16; i++)
//     *pdata++ = 100*sin(M_PI*time(NULL)/60+i/2.0)+100;

//   bk_close(pevent, pdata);

//   return bk_size(pevent);
// }


#ifdef STANDALONE
int main(int argc, char** argc) {

  char error[100];
  begin_of_run(10, error);
}
#endif
