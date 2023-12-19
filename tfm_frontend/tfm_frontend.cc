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
// #include "experim.h"

#include "mfe.h"
#include "artdaq/ExternalComms/MakeCommanderPlugin.hh"
#include "artdaq/Application/LoadParameterSet.hh"
#include "proto/artdaqapp.hh"

#include "xmlrpc-c/config.h"  /* information about this build environment */
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "tfm_frontend/db_runinfo.hh"
#include "tfm_frontend/tfm_frontend.hh"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = "tfm_frontend";
/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = FALSE;

// display frequency in ms for the frontend status page
INT display_period      = 3000;

/* maximum event size produced by this frontend */
// INT max_event_size = 1024 * 1024; // 1 MB
INT max_event_size = 1000; // 1 MB

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

static bool           _useRunInfoDB(false);
static xmlrpc_env     _env;


static void die_if_fault_occurred(xmlrpc_env * const envP) {
  if (envP->fault_occurred) {
    fprintf(stderr, "XML-RPC Fault: %s (%d)\n",envP->fault_string, envP->fault_code);
    exit(1);
  }
}

//-----------------------------------------------------------------------------
// the farm should be started independent on the frontend (or not ?)
// print message and return FE_ERR_HW if frontend should not be started 
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int         argc;
  char**      argv;
  std::string fcl_fn;
  HNDLE       hDB; // , hKey;

  mfe_get_args(&argc,&argv);
  
  if ((argc == 3) and (strcmp(argv[1],"-c") == 0)) {
//-----------------------------------------------------------------------------
// assume FCL file : 'tfm_frontend -c xxx.fcl'
//-----------------------------------------------------------------------------
    fcl_fn = argv[2];
  }
  else {
//-----------------------------------------------------------------------------
// figure out configuration in ODB
// active configuration ahs to be stored
//-----------------------------------------------------------------------------
    cm_get_experiment_database(&hDB, NULL);
    char  active_conf[100];
    int   sz = sizeof(active_conf);

    db_get_value(hDB, 0, "/Experiment/ActiveConfiguration", &active_conf, &sz, TID_STRING, TRUE);
    fcl_fn = "./config/";
    fcl_fn += active_conf;
    fcl_fn += "/tfm_frontend.fcl";
  }

  printf("tfm_frontend::%s fcl_fn=%s\n",__func__,fcl_fn.data());

  fhicl::ParameterSet top_ps = LoadParameterSet(fcl_fn);
  fhicl::ParameterSet tfm_ps = top_ps.get<fhicl::ParameterSet>("tfm_frontend",fhicl::ParameterSet());

  _useRunInfoDB = tfm_ps.get<bool>("useRunInfoDB" ,false);
  _xmlrpcUrl    = tfm_ps.get<std::string>("xmlrpcUrl","undefined");

  printf("tfm_frontend::%s _useRunInfoDB=%d\n",__func__,_useRunInfoDB);
  printf("tfm_frontend::%s _xmlrpcUrl   =%s\n",__func__,_xmlrpcUrl.data());
//-----------------------------------------------------------------------------
// this is for later - when we learn how to communicate with the TF manager
// properly
//-----------------------------------------------------------------------------
  // std::unique_ptr<artdaq::Commandable> comm =  std::make_unique<artdaq::Commandable>();
  // _commander = artdaq::MakeCommanderPlugin(ps, *comm);
  // _commander->run_server();

//-----------------------------------------------------------------------------
// init XML RPC
//-----------------------------------------------------------------------------
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "tfm_frontend", "v1_0");
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
// wait until the farm reports reaching a certain state
//-----------------------------------------------------------------------------
int wait_for(const char* State, int MaxWaitingTime) {
  int         rc (0);

  std::string         state;

  xmlrpc_value*       resultP;
  size_t              length;
  const char*         value;

  int                 waiting_time = 0;

  while (state != State) {

    resultP = xmlrpc_client_call(&_env, 
                                 _xmlrpcUrl.data(),
                                 "get_state",
                                 // "({s:i,s:i})",
                                 "(s)", 
                                 "daqint");
    die_if_fault_occurred(&_env);

    xmlrpc_read_string_lp(&_env, resultP, &length, &value);
    state = value;

    // xmlrpc_DECREF(resultP);
    // xmlrpc_env_clean(&_env);

    sleep(1);
    printf(" --- waiting: state=%s\n",state.data());
    waiting_time += 1;
    if (waiting_time > MaxWaitingTime) {
      rc = -1;
      break;
    }
  }

  printf("tfm_frontend::%s 001: FINISHED, rc=%i\n",__func__,rc);

  return rc;
}


//-----------------------------------------------------------------------------
// put here clear scalers etc.
//-----------------------------------------------------------------------------
INT begin_of_run(INT RunNumber, char *error) {

  xmlrpc_env    env;
  xmlrpc_value* resultP;

  xmlrpc_env_init(&env);
  resultP = xmlrpc_client_call(&env, 
                               _xmlrpcUrl.data(),
                               "state_change",
                               // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "configuring",
                               "run_number", RunNumber);
  die_if_fault_occurred(&_env);

  const char* value;
  size_t      length;
  xmlrpc_read_string_lp(&_env, resultP, &length, &value);
    
  std::string res = value;

  printf("tfm_frontend::%s  after XMLRPC command: result:%s\n",__func__,res.data());

  xmlrpc_DECREF   (resultP);
//-----------------------------------------------------------------------------
// now wait till completion
//-----------------------------------------------------------------------------
  int rc(0);

  rc = wait_for("configured:100",100);

  printf("tfm_frontend::%s 0011: DONE configuring, rc=%i\n",__func__,rc);
//-----------------------------------------------------------------------------
// how do I know that the configure step suceeded ?
// have to wait and make sure ? wait for a message from the farm manager
//
// // assuming it was OK, 'start'
//-----------------------------------------------------------------------------
  resultP = xmlrpc_client_call(&env, 
                               _xmlrpcUrl.data(),
                               "state_change",
                               // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "starting",
                               "ignored_variable", 9999);
  die_if_fault_occurred(&_env);
  xmlrpc_DECREF(resultP);
//-----------------------------------------------------------------------------
// wait till the run start completion
//-----------------------------------------------------------------------------
  rc = wait_for("running:100",70);

  printf("tfm_frontend::%s ERROR: wait for running run=%6i rc=%i\n",__func__,RunNumber,rc);

  if (_useRunInfoDB) {
    try { 
      db_runinfo db("aaa");
      rc = db.registerTransition(RunNumber,db_runinfo::START);
    }
    catch(char* err) {
      printf("tfm_frontend::%s : %s\n",__func__,err);
    }
  }
  printf("tfm_frontend::%s 003: done starting, run=%6i rc=%i\n",__func__,RunNumber,rc);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
INT end_of_run(INT RunNumber, char *Error) {
//-----------------------------------------------------------------------------
// first 'stop'
//-----------------------------------------------------------------------------
  printf("tfm_frontend::%s 000: trying to STOP run=%6i\n",__func__,RunNumber);

  xmlrpc_value* resultP;
  size_t        length;
  const char*   value;

  resultP = xmlrpc_client_call(&_env, 
                               _xmlrpcUrl.data(),
                               "state_change",
                               // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "stopping",
                               "ignored_variable", 9999);
  die_if_fault_occurred(&_env);

  xmlrpc_read_string_lp(&_env, resultP, &length, &value);
  std::string result = value;

  xmlrpc_DECREF(resultP);
  //  xmlrpc_env_clean(&_env);

  printf("tfm_frontend::%s after my_xmlrpc command: run=%6i result:%s\n",__func__,RunNumber,result.data());
//-----------------------------------------------------------------------------
// now wait till completion
//-----------------------------------------------------------------------------
  int rc(0);

  printf("tfm_frontend::%s: wait for completion\n",__func__);

  rc = wait_for("stopped:100",100);

  printf("tfm_frontend::%s : DONE STOPPING, rc=%i\n",__func__,rc);
//-----------------------------------------------------------------------------
// write end of transition into the DB
//-----------------------------------------------------------------------------
  if (_useRunInfoDB) {
    db_runinfo db("aaa");
    rc = db.registerTransition(RunNumber,db_runinfo::STOP);
    if (rc < 0) {
      printf("tfm_frontend::%s ERROR: failed to regiser STOP transition rc=%i",__func__,rc);
      sprintf(Error,"tfm_frontend::%s ERROR: line %i failed to regiser STOP transition",__func__,__LINE__);
    }
  }

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
