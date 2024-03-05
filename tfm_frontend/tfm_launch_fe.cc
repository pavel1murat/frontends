//------------------------------------------------------------------------------
// P.Murat: this frontend just launches the farm manager
// make sure to use the correct TRACE name 
// the host name has to be specified upon submission
// start: tfm_launch_fe -h mu2edaq09.fnal.gov
// host_name is defined in midas/include/mfe.h
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

#include "frontends/utils/utils.hh"
#include "frontends/utils/db_runinfo.hh"
#include "frontends/tfm_frontend/tfm_launch_fe.hh"

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
//static char          _artdaq_conf[100];
static char          _xmlrpcUrl  [100];
static xmlrpc_env    _env;
static int           _init(0);
// static bool          _use_screen(true);   // using screen anyway
static bool          _tfm_messages(false);

//-----------------------------------------------------------------------------
// the farm should be started independent on the monitoring frontend
// print message and return FE_ERR_HW if frontend should not be started 
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int         argc;
  char**      argv;
  std::string fcl_fn;
  HNDLE       hDB, hKey, h_active_run_conf;
  char        active_conf[100];

  set_equipment_status(equipment[0].name, "Initializing...", "yellow");
  TLOG(TLVL_DEBUG+4) << "starting";

  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);
  int   sz = sizeof(active_conf);
  db_get_value(hDB, 0, "/Mu2e/ActiveRunConfiguration", &active_conf, &sz, TID_STRING, TRUE);

  char key[200];
  sprintf(key,"/Mu2e/RunConfigurations/%s",active_conf);
	db_find_key(hDB, 0, key, &h_active_run_conf);
//-----------------------------------------------------------------------------
// check whether or not use run number from the DB
//-----------------------------------------------------------------------------
  sz = sizeof(_useRunInfoDB);
  db_get_value(hDB, h_active_run_conf, "UseRunInfoDB", &_useRunInfoDB, &sz, TID_BOOL, TRUE);
//-----------------------------------------------------------------------------
// handle the name of the host we'running
//-----------------------------------------------------------------------------
  std::string host = get_full_host_name("local");
//-----------------------------------------------------------------------------
// ARTDAQ_PARTITION_NUMBER and configuration name are defined in the active run configuration
// TF manager port is defined by the partition number
//-----------------------------------------------------------------------------
  sz = sizeof(_partition);
  db_get_value(hDB, 0, "/Mu2e/ARTDAQ_PARTITION_NUMBER", &_partition, &sz, TID_INT32, TRUE);
  _port = 10000+1000*_partition;
  sprintf(_xmlrpcUrl,"http://%s:%i/RPC2",host.data(),_port);

  // sz = sizeof(_use_screen);
  // sprintf(key,"/Equipment/%s/UseScreen", equipment[0].name);
  // db_get_value(hDB, 0, key, &_use_screen, &sz, TID_BOOL, TRUE);

  sz = sizeof(_tfm_messages);
  sprintf(key,"/Equipment/%s/CheckTfmMessages", equipment[0].name);
  db_get_value(hDB, 0, key, &_tfm_messages, &sz, TID_BOOL, TRUE);

  cm_msg(MINFO,"debug","_tfm_messages: %i", _tfm_messages);
  TLOG(TLVL_DEBUG+4) << "farm_manager _useRunInfoDB:" << _useRunInfoDB;
  TLOG(TLVL_DEBUG+4) << "farm_manager _xmlrpcUrl   :" << _xmlrpcUrl ;
//-----------------------------------------------------------------------------
// Create or clear the "/Equipment/tfm_launch/Clients" ODB sub-tree
//-----------------------------------------------------------------------------
  sprintf(key,"/Equipment/%s/Clients", equipment[0].name); 
  if(db_find_key(hDB, 0, key, &hKey) == DB_SUCCESS) {
     db_delete_key(hDB, hKey, false);
  } 
  db_create_key(hDB, 0, key, TID_KEY);
  //db_find_key(hDB, 0, str, hKey);
//-----------------------------------------------------------------------------
// launch the farm manager via screen
//-----------------------------------------------------------------------------
  cm_msg(MINFO, "tfm_manager", "Launch the farm manager with run config '%s', partition %i", 
         active_conf, _partition);
  char cmd[200];

  sprintf(cmd,"screen -dmS tfm_%i bash -c \"daq_scripts/start_farm_manager %s %i\"",
          _partition,active_conf,_partition);
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
//-----------------------------------------------------------------------------
// clean up "Clients" in ODB, remove them all
//-----------------------------------------------------------------------------
  HNDLE hDB, hKey;
  char key[200];
  cm_get_experiment_database(&hDB, NULL); 
  sprintf(key,"/Equipment/%s/Clients", equipment[0].name); 
  if(db_find_key(hDB, 0, key, &hKey) == DB_SUCCESS) {
     db_delete_key(hDB, hKey, false);
  } 
  db_create_key(hDB, 0, key, TID_KEY);

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
//-----------------------------------------------------------------------------
// register beginning of the transition 
//-----------------------------------------------------------------------------
  int rc(0);
  if (_useRunInfoDB) {
    try { 
      db_runinfo db("aaa");
      rc = db.registerTransition(RunNumber,db_runinfo::START,0);
    }
    catch(char* err) {
      TLOG(TLVL_ERROR) << "failed to register beginning of the START transition rc=" << rc;
    }
  }

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
//-----------------------------------------------------------------------------
// register the end of the start completion
//-----------------------------------------------------------------------------
  if (_useRunInfoDB) {
    try { 
      db_runinfo db("aaa");
      rc = db.registerTransition(RunNumber,db_runinfo::START,1);
    }
    catch(char* err) {
      TLOG(TLVL_ERROR) << "failed to register end of the START transition rc=" << rc;
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
//-----------------------------------------------------------------------------
// write beginning of the transition into the DB
//-----------------------------------------------------------------------------
  int rc(0);

  if (_useRunInfoDB) {
    db_runinfo db("aaa");
    rc = db.registerTransition(RunNumber,db_runinfo::STOP,0);
    if (rc < 0) {
      TLOG(TLVL_ERROR) << "failed to register beginning of the END_RUN transition rc=" << rc;
    }
  }
//-----------------------------------------------------------------------------
// send the transition request 
//-----------------------------------------------------------------------------
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

  TLOG(TLVL_INFO) << "after my_xmlrpc command run=" << RunNumber << "result:" << result;
//-----------------------------------------------------------------------------
// now wait till completion
//-----------------------------------------------------------------------------
  TLOG(TLVL_DEBUG) << "wait for completion";

  rc = wait_for("stopped:100",100);

  TLOG(TLVL_DEBUG) << "DONE stopping rc=" << rc;
//-----------------------------------------------------------------------------
// write end of STOP_RUN transition into the DB
//-----------------------------------------------------------------------------
  if (_useRunInfoDB) {
    db_runinfo db("aaa");
    rc = db.registerTransition(RunNumber,db_runinfo::STOP,1);
    if (rc < 0) {
      TLOG(TLVL_ERROR) << "failed to register end of the STOP_RUN transition rc=" << rc;
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
  if (_init > 0) {

    //    int         rc (0);

    std::string         state;
    xmlrpc_env          env;
    xmlrpc_value*       resultP;
    size_t              length;
    const char*         value;
    
    try {
      xmlrpc_env_init(&env);
//-----------------------------------------------------------------------------
// 'daqint' here is the name of the FarmManager (former artdaq_DAQInterface) 
// doesn't seem to be used anywhere, but still have to pass (or change the interfaces)
//-----------------------------------------------------------------------------
                                        // "({s:i,s:i})",
      resultP = xmlrpc_client_call(&env,_xmlrpcUrl,"get_state","(s)","daqint");

  //int                 waiting_time = 0;

      xmlrpc_env_init(&env);
      resultP = xmlrpc_client_call(&env, 
                                   _xmlrpcUrl,
                                   "get_state",
                                   // "({s:i,s:i})",
                                   "(s)", 
                                   "daqint");
      if (env.fault_occurred) {
        TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << "."; 
        //cm_msg(MERROR,"tfm","XML-RPC rc=%i %s", env.fault_code, env.fault_string); 
        set_equipment_status(equipment[0].name, "booting", "yellow");
      } 
      else {
        
        xmlrpc_read_string_lp(&env, resultP, &length, &value);
        state = value;
  
        set_equipment_status(equipment[0].name, state.c_str(), "greenLight");
        //cm_msg(MINFO,"tfm", "TFM state: '%s'", state.c_str());
//-----------------------------------------------------------------------------
// only if getting the status of the farm manager worked, try update the status of all farm processes here
// P.M. this is an idea of Simon - this idea is good
// the command returns status of all artdaq processes - putting all into one string 
// could be a bit of an overkill
// what if the number of clients is too large ? - how does this approach scale ?
// this is how a very long string shows up on the front page
//-----------------------------------------------------------------------------
        resultP = xmlrpc_client_call(&env, 
                                     _xmlrpcUrl,
                                     "artdaq_process_info",
                                     // "({s:i,s:i})",
                                     "(s)", 
                                     "daqint");
        if (env.fault_occurred) {
          TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << ".";
          cm_msg(MERROR,"frontend_loop","Failed to `artdaq_process_info` with '%s'.", env.fault_string); 
        } 
        else {
//-----------------------------------------------------------------------------
// trying to parse potentially long output 'artdaq_process_info'
//-----------------------------------------------------------------------------
          xmlrpc_read_string_lp(&env, resultP, &length, &value);
          std::string artdaq_process_info = value;
          std::istringstream iss(artdaq_process_info);
          std::string line;
          
          const char *client_str =
"[.]\n\
host      = STRING : [64] \n\
status    = STRING : [64] \n\
subsystem = INT : 0\n\
rank      = INT : 0";
        
          typedef struct {
            //char name[64];
            char host  [64];
            char status[64];
            INT  subsystem;
            INT  rank;
          } client_t;
          client_t client;
          
          std::string name;
          
          HNDLE hDB, hKey, hKeyClient;
          cm_get_experiment_database(&hDB, NULL);
          char key[200]; 
          sprintf(key,"/Equipment/%s/Clients", equipment[0].name);
          db_find_key(hDB, 0, key, &hKey); // check if found?
          
          int cnt = 0;
          while (std::getline(iss, line)) {
            cnt++;
            // 
            std::istringstream words(line);
            std::string word;
            int wcnt = 0;
            strcpy(client.status, "n/a");
            while (getline(words, word,' ')) { 
              if (wcnt == 0) name             = word;
              if (wcnt == 2) strcpy(client.host, word.c_str());
              if (wcnt == 4) client.subsystem = std::stoi(word.substr(0, word.size()-1));
              if (wcnt == 6) client.rank      = std::stoi(word.substr(0, word.size()-2));
              if (wcnt == 7) strcpy(client.status, word.c_str());
              wcnt++;
            }

            if(wcnt != 8) { // check that the word count is write, if not, something is off
              cm_msg(MERROR,"frontend_loop","Parsing '%s' failed with wrong word count of %i. Expect 8.", name.c_str(), wcnt);
              strcpy(client.status, "read error");
            }
//-----------------------------------------------------------------------------
// check if the client already exists, if not add it
//-----------------------------------------------------------------------------
            sprintf(key,"%s", name.c_str());
            if(db_find_key(hDB, hKey, key, &hKeyClient) != DB_SUCCESS) {
              db_create_record(hDB, hKey, key, client_str);
              db_find_key(hDB, hKey, key, &hKeyClient);
            }
            
            // update the client's status 
           db_set_record(hDB, hKeyClient, &client, sizeof(client_t), 0);
          }
        }
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
        if (_tfm_messages) {
          // check for new messages
          resultP = xmlrpc_client_call(&env, 
                                       _xmlrpcUrl,
                                       "get_messages",
                                       "(s)",
                                       "daqint");
          if (env.fault_occurred) {
            TLOG(TLVL_ERROR) << "XML-RPC rc=" << env.fault_code << " " << env.fault_string << ".";
            cm_msg(MERROR,"frontend_loop","Failed to `get_messages` with '%s'.", env.fault_string); 
          }
          else {
            xmlrpc_read_string_lp(&env, resultP, &length, &value);
            std::string messages = value;
            if (! messages.empty()) {
              std::istringstream iss(messages);
              std::string line;
              size_t cnt = 0;
              while (std::getline(iss, line)) {
                std::cout << cnt << std::endl;
                if (cnt++ > 3) {
                  cm_msg(MINFO,"artdaq","Additional messages were suppressed.");
                }
                size_t pos = line.find(":");
                if(line.substr(0, pos) == "error") {
                  cm_msg(MERROR,"artdaq","%s", line.substr(pos+1).c_str());
                } 
                else if(line.substr(0, pos) == "alarm") {
                  al_trigger_alarm("artdaq", line.substr(pos+1).c_str(), "artdaq", "unhappy", AT_INTERNAL);
                  //cm_msg(MERROR,"artdaq","%s", state.substr(pos+1).c_str()); // todo, midas alarm
                } 
                else {
                  cm_msg(MINFO,"artdaq","%s", line.substr(pos+1).c_str());
                }
              }
            }
          }
        }
      }
      if (env.fault_occurred) {
        TLOG(TLVL_ERROR) << "001: XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING..."; 
        // cm_msg(MERROR,"tfm","XML-RPC rc=%i %s", env.fault_code, env.fault_string); 
        set_equipment_status(equipment[0].name, "booting", "greenLight");
      } 
      else {
//-----------------------------------------------------------------------------
// everything looks OK, define the frontend state (status)
//-----------------------------------------------------------------------------
      resultP = xmlrpc_client_call(&env, 
                                   _xmlrpcUrl,
                                   "get_state",
                                   // "({s:i,s:i})",
                                   "(s)", 
                                   "daqint");
        xmlrpc_read_string_lp(&env, resultP, &length, &value);
//-----------------------------------------------------------------------------
// I think this is the place where a long string gets into the status
//-----------------------------------------------------------------------------
        std::string fe_state = value;
        set_equipment_status(equipment[0].name, fe_state.data(), "greenLight");
        //cm_msg(MINFO,"tfm", "TFM state: '%s'", state.c_str()); 
      }
    }
    catch(...) {
      TLOG(TLVL_ERROR) << "002: XML-RPC rc=" << env.fault_code << " " << env.fault_string << " EXITING..."; 
    }
  } 
//-----------------------------------------------------------------------------
// sleeep for a second
//-----------------------------------------------------------------------------
  ss_sleep(1000);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
// Readout routines for different event types
//-----------------------------------------------------------------------------
/*-- Dummy routines ------------------------------------------------*/
INT poll_event(INT source, INT count, BOOL test) {
  return 1;
}


//-----------------------------------------------------------------------------
INT interrupt_configure(INT cmd, INT source, POINTER_T adr) {
  return 1;
}


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
