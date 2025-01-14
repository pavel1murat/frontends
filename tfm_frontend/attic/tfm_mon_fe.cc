///////////////////////////////////////////////////////////////////////////////
// trigger farm monitoring frontend
// this is a monitoring frontend running on a single node
// try to make it work
// frontend needs to know on which node it is running, so always use -h flag
// start : 'tfm_mon_fe -h mu2edaq09.fnal.gov'
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

#include "utils/utils.hh"
#include "utils/OdbInterface.hh"
#include "tfm_frontend/tfm_mon_fe.hh"
//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
const char *frontend_name      = TRACE_NAME; // frontend name (client name) as seen by other MIDAS clients 
const char *frontend_file_name = __FILE__  ; // frontend file name, don't change
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

namespace {
  xmlrpc_env     _env;
  //  char           _artdaq_conf[50];
}

// DEVICE_DRIVER driver_list[] = {
//   {"tfm_driver"   , tfm_driver   , TFM_DRIVER_NWORDS   , null, DF_INPUT},
//   {"tfm_br_driver", tfm_br_driver, TFM_BR_DRIVER_NWORDS, null, DF_INPUT},
//   {""}
// };

namespace {
  DEVICE_DRIVER* _driver_list(nullptr);
}
//-----------------------------------------------------------------------------
// the farm should be started independent on the frontend (or not ?)
// print message and return FE_ERR_HW if frontend should not be started 
// P.M. use _argc and _argv provided by Stefan
//-----------------------------------------------------------------------------
INT frontend_init() {
  int         argc;
  char**      argv;
  HNDLE       hDB;

  mfe_get_args(&argc,&argv);
//-----------------------------------------------------------------------------
// figure out the active configuration from ODB
//-----------------------------------------------------------------------------
  cm_get_experiment_database(&hDB, NULL);

  OdbInterface* odb_i             = OdbInterface::Instance(hDB);
  HNDLE         h_active_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   private_subnet    = odb_i->GetPrivateSubnet(h_active_run_conf);
  std::string   public_subnet     = odb_i->GetPublicSubnet (h_active_run_conf);
  std::string   active_run_conf   = odb_i->GetRunConfigName(h_active_run_conf);
  if(h_active_run_conf == 0) {
    TLOG(TLVL_ERROR) << "Configuration " << active_run_conf << " was not found in /Mu2e/RunConfigurations. BAIL OUT";
    return FE_ERR_ODB; 
  }
//-----------------------------------------------------------------------------
// ARTDAQ_PARTITION_NUMBER also comes from the active run configuration
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// get port number used by the TF manager 
// assume the farm_manager is running locally
//-----------------------------------------------------------------------------
  int partition                 = odb_i->GetArtdaqPartition();
  int port_number               = 10000+1000*partition;
  std::string private_host_name = get_full_host_name(private_subnet.data());
  _xmlrpcUrl = std::format("http://{}:{}/RPC2",private_host_name.data(),port_number);

  TLOG(TLVL_DEBUG+4) << "farm_manager _xmlrpcUrl   :" << _xmlrpcUrl ;
//-----------------------------------------------------------------------------
// this is for later - when we learn how to communicate with the TF manager
// properly
//
// init XML RPC
//-----------------------------------------------------------------------------
  xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, "tfm_mon_fe", "v1_0");
  xmlrpc_env_init(&_env);
//-----------------------------------------------------------------------------
// try to initialize the driver list dynamically. 
// 1. First try a simple thing and it seems to work
// 2. next, loop over components in the detector configuration and create 
//    a driver for each one
// find ARTDAQ configuration , 'host_name' (hostname, i.e. mu2edaq09.fnal.gov) 
// is a global from midas/src/mfe.cxx and should be defined on the command line! 
//-----------------------------------------------------------------------------
  std::string host_label = get_short_host_name(public_subnet.data());
  HNDLE h_host_artdaq_conf = odb_i->GetHostArtdaqConfHandle(h_active_run_conf,host_label);

  if (h_host_artdaq_conf == 0) {
    TLOG(TLVL_ERROR) << "no ARTDAQ configuration for host:" << active_run_conf << ":" << host_label;
    return FE_ERR_ODB; 
  }
//-----------------------------------------------------------------------------
// got active artdaq configuration/host
// first, figure out the number of components
//-----------------------------------------------------------------------------
  HNDLE h_component;
  KEY   component;
  int   ncomp(0);
  for (int i=0; db_enum_key(hDB, h_host_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
    db_get_key(hDB, h_component, &component);
    printf("Subkey %d: %s, Type: %d\n", i, component.name, component.type);
    ncomp++;
  }
//-----------------------------------------------------------------------------
// knowing the number of components, create a driver list
// the total number of "drivers" should be ncomp+1 (including the "disk driver")
// nothing for dispatcher so far
//-----------------------------------------------------------------------------
  DEVICE_DRIVER* x = new DEVICE_DRIVER[ncomp+1];
  _driver_list = x;
  
  for (int i=0; db_enum_key(hDB, h_host_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
//-----------------------------------------------------------------------------
// for each active component, define a driver
// use the component label 
// so far, output of all drivers goes into the same common "Input" array
// component names:
//                   brxx - board readers
//                   ebxx - event builders
//                   dlxx - data loggers
//                   dsxx - dispatchers
//-----------------------------------------------------------------------------
    db_get_key(hDB, h_component, &component);

    // char label[NAME_LENGTH];
    // sz = sizeof(label);
    // db_get_value(hDB, h_component, "Label", &label, &sz, TID_STRING, FALSE);

    DEVICE_DRIVER driver;
    if (strstr(component.name,"br") == component.name) {
      snprintf(driver.name,NAME_LENGTH,"%s",component.name);
      driver.dd       = tfm_br_driver;               //  this is a function ..
      driver.channels = TFM_BR_DRIVER_NWORDS;
      driver.bd       = null;
      driver.flags    = DF_INPUT;
      _driver_list[i] = driver;
    }
    else if (strstr(component.name,"eb") == component.name) {
      snprintf(driver.name,NAME_LENGTH,"%s",component.name);
      driver.dd       = tfm_dr_driver;
      driver.channels = TFM_DR_DRIVER_NWORDS;
      driver.bd       = null;
      driver.flags    = DF_INPUT;
      _driver_list[i] = driver;
    }
    else if (strstr(component.name,"dl"  ) == component.name) {
      snprintf(driver.name,NAME_LENGTH,"%s",component.name);
      driver.dd       = tfm_dr_driver;
      driver.channels = TFM_DR_DRIVER_NWORDS;
      driver.bd       = null;
      driver.flags    = DF_INPUT;
      _driver_list[i] = driver;
    }
    else if (strstr(component.name,"ds"  ) == component.name) {
      // so far, do nothing
    }
  }
//-----------------------------------------------------------------------------
// disk reporting , nothing for dispatcher, so ncomp-1
//-----------------------------------------------------------------------------
  DEVICE_DRIVER* drv = &_driver_list[ncomp-1];
  snprintf(drv->name,NAME_LENGTH,"%s","diskmon");
  drv->dd            = tfm_disk_driver;
  drv->channels      = TFM_DISK_DRIVER_NWORDS;
  drv->bd            = null;
  drv->flags         = DF_INPUT;
//-----------------------------------------------------------------------------
// end of the list - a driver with an empty name
//-----------------------------------------------------------------------------
  _driver_list[ncomp].name[0] = 0;

  equipment[0].driver = _driver_list;

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
