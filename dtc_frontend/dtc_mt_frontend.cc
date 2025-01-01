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
#define  TRACE_NAME "dtc_mt_frontend"

#include "dtc_frontend/TEquipmentNode.hh"

//-----------------------------------------------------------------------------
static void usage() {
   fprintf(stderr, "Usage: tmfe_example_mt ...\n");
   exit(1);
}

//-----------------------------------------------------------------------------
class DtcFrontend: public TMFrontend {
public:
  
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  DtcFrontend(const char* Name);
  
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
  void HandleFrontendExit() {} //printf("FeEverything::HandleFrontendExit!\n");  }
};

//-----------------------------------------------------------------------------
// add future node fronten, 
// the base class constructor does nothing except instantiating TFME thing
// all parameters need to be initalized here
//-----------------------------------------------------------------------------
DtcFrontend::DtcFrontend(const char* Name) : TMFrontend() {
  
  FeSetName(Name);
  
  TEquipmentNode* eq_dtc = new TEquipmentNode("mu2edaq22",__FILE__);

// add eq to the list of equipment pieces
// equipment stores backward pointer to the frontend
  FeAddEquipment(eq_dtc);
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

  DtcFrontend fe("dtc_mt_frontend");

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
