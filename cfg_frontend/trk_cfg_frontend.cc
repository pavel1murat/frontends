///////////////////////////////////////////////////////////////////////////////
// cloned from midas/progs/tmfe_example_multithread.cxx
//
// Example tmfe multithreaded c++ frontend
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <signal.h> // SIGPIPE
#include <assert.h> // assert()
#include <stdlib.h> // malloc()
#include <unistd.h> // sleep()
#include <math.h>   // M_PI

#include "midas.h"
#include "tmfe.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "trk_cfg_frontend"

#include "cfg_frontend/TEquipmentTracker.hh"
#include "utils/utils.hh"

//-----------------------------------------------------------------------------
class TrackerCfgFrontend: public TMFrontend {
public:

  std::string            fName;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TrackerCfgFrontend();
  
  void HandleUsage() {}   //printf("FeEverything::HandleUsage!\n");  }
  
  TMFeResult HandleArguments(const std::vector<std::string>& args) {
    //printf("FeEverything::HandleArguments!\n");
    return TMFeOk();
  }
   
  TMFeResult HandleFrontendInit(const std::vector<std::string>& args) {
    //printf("FeEverything::HandleFrontendInit!\n");
    return TMFeOk();
  }
   
  TMFeResult HandleFrontendReady(const std::vector<std::string>& args) {
    //printf("FeEverything::HandleFrontendReady!\n");
    //FeStartPeriodicThread();
    //fMfe->StartRpcThread();
    return TMFeOk();
  }

  bool ShutdownRequested () { return fMfe->fShutdownRequested ; }
  void Disconnect        () { fMfe->Disconnect(); }
  void HandleFrontendExit() {} //printf("FeEverything::HandleFrontendExit!\n");
};

//-----------------------------------------------------------------------------
// add future node frontend, 
// the base class constructor does nothing except instantiating the TFME thing
// but that doesn't connect to ODB yet...
// all parameters need to be initalized here
//-----------------------------------------------------------------------------
TrackerCfgFrontend::TrackerCfgFrontend() : TMFrontend() {

  std::string hostname = get_short_host_name("");
  fName  = "trk_config";
  FeSetName(fName.data());
  
// add eq to the list of equipment pieces, equipment stores backward pointer to the frontend
  TEquipmentTracker* eq = new TEquipmentTracker("tracker",__FILE__);
  FeAddEquipment(eq);
//-----------------------------------------------------------------------------
// TRK config frontend starts after the DTC frontends (500) and stops before them
//-----------------------------------------------------------------------------
  cm_set_transition_sequence(TR_START,505);
  cm_set_transition_sequence(TR_STOP ,495);
}


//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  signal(SIGPIPE, SIG_IGN);
  
  //std::string name = "";
  //
  //if (argc == 2) {
  //   name = argv[1];
  //} else {
  //   usage(); // DOES NOT RETURN
  //}

  TrackerCfgFrontend fe;

  // FeMain calls FeInit - at this point connection to the experiment happens
  // this is too late for equipment to be initialized in the constructor
  //   - however, after connecting, FeInit calls FeInitEquipments which loops
  //     over equipment pieces and calls EqInit function for each of them
  // and after that goes into the FeMainLoop
  // in the end, it calls TMFE::Disconnect which calls cm_disconnect_experiment 
  return fe.FeMain(argc,argv);
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
