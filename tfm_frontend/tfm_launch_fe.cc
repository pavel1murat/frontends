//------------------------------------------------------------------------------
// P.Murat: this frontend just launches the farm manager
// make sure the TRACE name is the same as the 
//-----------------------------------------------------------------------------
#include "TRACE/tracemf.h"
#define  TRACE_NAME "tfm_launch_fe"

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
#include "tfm_frontend/tfm_launch_fe.hh"

/*-- Globals -------------------------------------------------------*/

/* The frontend name (client name) as seen by other MIDAS clients   */
const char *frontend_name = TRACE_NAME;

/* The frontend file name, don't change it */
const char *frontend_file_name = __FILE__;

/* frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

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
static uint          _useRunInfoDB(0);
static int           _partition(0);
static int           _port(0);
static char          _artdaq_conf[100];
static char          _xmlrpcUrl  [100];
static xmlrpc_env    _env;
static int           _init(0);
static bool          _use_screen(false);

//-----------------------------------------------------------------------------
// the farm should be started independent on the monitoring frontend
// print message and return FE_ERR_HW if frontend should not be started 
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int         argc;
  char**      argv;
  std::string fcl_fn;
  HNDLE       hDB, hKey;
  char        active_conf[100];

  set_equipment_status(equipment[0].name, "Initializing...", "yellow");
  TLOG(TLVL_DEBUG+4) << "starting";

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
// check whether or not use run number from the DB
//-----------------------------------------------------------------------------
  sz = sizeof(_useRunInfoDB);
  db_get_value(hDB, hKey, "UseRunInfoDB", &_useRunInfoDB, &sz, TID_BOOL, TRUE);
//-----------------------------------------------------------------------------
// ARTDAQ_PARTITION_NUMBER and configuration name are defined in the active run configuration
// TF manager port is defined by the partition number
//-----------------------------------------------------------------------------
  sz = sizeof(_partition);
  db_get_value(hDB, hKey, "ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
  _port = 10000+1000*_partition;
  sprintf(_xmlrpcUrl,"http://localhost:%i/RPC2",_port);

  sz = sizeof(_artdaq_conf);
  db_get_value(hDB, hKey, "ArtdaqConfiguration", _artdaq_conf, &sz, TID_STRING, TRUE);

  sz = sizeof(_use_screen);
  sprintf(key,"/Equipment/%s/UseScreen", equipment[0].name);
  db_get_value(hDB, 0, key, &_use_screen, &sz, TID_BOOL, TRUE);

  cm_msg(MINFO,"debug","_use_screen: %i", _use_screen);
  TLOG(TLVL_DEBUG+4) << "farm_manager _useRunInfoDB:" << _useRunInfoDB;
  TLOG(TLVL_DEBUG+4) << "farm_manager _xmlrpcUrl   :" << _xmlrpcUrl ;
//-----------------------------------------------------------------------------
// launch the farm manager via dbus-console
//-----------------------------------------------------------------------------
  cm_msg(MINFO, "tfm_manager", "Launch the farm manager with run config '%s', artdaq config '%s', partition %i", 
         active_conf, _artdaq_conf, _partition);
  char cmd[200];
  sprintf(cmd,"dbus-launch konsole -p tabtitle=farm_manager -e daq_scripts/start_farm_manager %s %i",
          _artdaq_conf,_partition);
  if(_use_screen) {
    sprintf(cmd,"screen -dmS tfm bash -c \"daq_scripts/start_farm_manager %s %i\"",
            _artdaq_conf,_partition); 
  }
  TLOG(TLVL_DEBUG+4) << "before launching: cmd=" << cmd;
  system(cmd);
  TLOG(TLVL_DEBUG+4) << "after launching: cmd=" << cmd;
  
//-----------------------------------------------------------------------------
// init XML RPC
//-----------------------------------------------------------------------------
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, frontend_name, "v1_0");
  xmlrpc_env_init(&_env);

  set_equipment_status(equipment[0].name, "OK", "green");
  _init = 1;
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// on exit, shutdown the farm_manager
//-----------------------------------------------------------------------------
INT frontend_exit() {
  _init = 0;
  xmlrpc_value* resultP;
  size_t        length;
  const char*   value;
 
  cm_msg(MINFO, "tfm_manager", "Shuting down the farm manager.");
  //cm_disconnect_experiment();
  resultP = xmlrpc_client_call(&_env,_xmlrpcUrl,"shutdown","(s)","daqint");

  if (_env.fault_occurred) {
    TLOG(TLVL_ERROR) << "XML-RPC rc=" << _env.fault_code << " " << _env.fault_string << " EXITING...";
    exit(1);
  }

  xmlrpc_read_string_lp(&_env, resultP, &length, &value);
  std::string result = value;

  xmlrpc_DECREF(resultP);

  TLOG(TLVL_INFO) << "after shutdown command, result:" << result;

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

  xmlrpc_env          env;
  xmlrpc_value*       resultP;
  size_t              length;
  const char*         value;

  int                 waiting_time = 0;

  xmlrpc_env_init(&env);

  while (state != State) {

    resultP = xmlrpc_client_call(&env, 
                                 _xmlrpcUrl,
                                 "get_state",
                                 // "({s:i,s:i})",
                                 "(s)", 
                                 "daqint");
    if (env.fault_occurred) {
      TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING...";
      return env.fault_code;
      exit(1);
    }

    xmlrpc_read_string_lp(&env, resultP, &length, &value);
    state = value;

    // xmlrpc_DECREF(resultP);
    // xmlrpc_env_clean(&_env);

    sleep(1);

    TLOG(TLVL_DEBUG) << "000:WAITING state=" << state;
    set_equipment_status(equipment[0].name, state.c_str(), "yellow");


    waiting_time += 1;
    if (waiting_time > MaxWaitingTime) {
      rc = -1;
      break;
    }
  }
  TLOG(TLVL_DEBUG) << "001 FINISHED rc=" << rc;

  return rc;
}

//-----------------------------------------------------------------------------
// move begin run functionality here
//-----------------------------------------------------------------------------
INT begin_of_run(INT RunNumber, char *error) {
  xmlrpc_env    env;
  xmlrpc_value* resultP;

  set_equipment_status(equipment[0].name, "Run starting...", "yellow");

  xmlrpc_env_init(&env);
  resultP = xmlrpc_client_call(&env, 
                               _xmlrpcUrl,
                               "state_change",
                                                         // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "configuring",
                               "run_number", RunNumber);
  if (env.fault_occurred) {
    TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING...";
    exit(1);
  }

  const char* value;
  size_t      length;
  xmlrpc_read_string_lp(&env, resultP, &length, &value);
    
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
                               _xmlrpcUrl,
                               "state_change",
                                                           // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "starting",
                               "ignored_variable", 9999);
  if (env.fault_occurred) {
    TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string;
    return env.fault_code;
  }

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
      TLOG(TLVL_ERROR) << "failed to register START transition rc=" << rc;
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
  TLOG(TLVL_DEBUG) << "000: trying to STOP run=" << RunNumber;

  set_equipment_status(equipment[0].name, "Run stopping...", "yellow");

  xmlrpc_env    env;
  xmlrpc_value* resultP;

  size_t        length;
  const char*   value;

  xmlrpc_env_init(&env);
  resultP = xmlrpc_client_call(&_env, 
                               _xmlrpcUrl,
                               "state_change",
                                                                                // "({s:i,s:i})",
                               "(ss{s:i})", 
                               "daqint",
                               "stopping",
                               "ignored_variable", 9999);
  if (env.fault_occurred) {
    TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING...";
    exit(1);
  }

  xmlrpc_read_string_lp(&env, resultP, &length, &value);
  std::string result = value;

  xmlrpc_DECREF(resultP);
  //  xmlrpc_env_clean(&_env);

  TLOG(TLVL_INFO) << "after my_xmlrpc command run=" << RunNumber << "result:" << result;
//-----------------------------------------------------------------------------
// now wait till completion
//-----------------------------------------------------------------------------
  int rc(0);

  TLOG(TLVL_DEBUG) << "wait for completion";

  rc = wait_for("stopped:100",100);

  TLOG(TLVL_DEBUG) << "DONE stopping rc=" << rc;
//-----------------------------------------------------------------------------
// write end of transition into the DB
//-----------------------------------------------------------------------------
  if (_useRunInfoDB) {
    db_runinfo db("aaa");
    rc = db.registerTransition(RunNumber,db_runinfo::STOP);
    if (rc < 0) {
      TLOG(TLVL_ERROR) << "failed to register STOP transition rc=" << rc;
      sprintf(Error,"tfm_frontend::%s ERROR: line %i failed to regiser STOP transition",__func__,__LINE__);
    }
  }


  set_equipment_status(equipment[0].name, "OK", "green");

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
  if(_init > 0) {

  int         rc (0);

  std::string         state;

  xmlrpc_env          env;
  xmlrpc_value*       resultP;
  size_t              length;
  const char*         value;

  int                 waiting_time = 0;

  xmlrpc_env_init(&env);
  resultP = xmlrpc_client_call(&env, 
                               _xmlrpcUrl,
                               "get_state",
                               // "({s:i,s:i})",
                               "(s)", 
                               "daqint");
  if (env.fault_occurred) {
    TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING..."; 
    //cm_msg(MERROR,"tfm","XML-RPC rc=%i %s", env.fault_code, env.fault_string); 
    set_equipment_status(equipment[0].name, "booting", "yellow");
  } else {

    xmlrpc_read_string_lp(&env, resultP, &length, &value);
    state = value;
  
    set_equipment_status(equipment[0].name, state.c_str(), "greenLight");
    //cm_msg(MINFO,"tfm", "TFM state: '%s'", state.c_str()); 
  }
  } 
  ss_sleep(1000);
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
