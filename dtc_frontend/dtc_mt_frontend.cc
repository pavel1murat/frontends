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
#include <math.h> // M_PI

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
//  add future DTC frontend
//-----------------------------------------------------------------------------
DtcFrontend::DtcFrontend(const char* Name) : TMFrontend() {
  FeSetName(Name);
  
  TEquipmentNode* eq_dtc = new TEquipmentNode("mu2edaq22",__FILE__);
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

  // this frontend connects twice, **TO BE FIXED**
  cm_connect_experiment(NULL, NULL, "Name", NULL);

  DtcFrontend fe("dtc_mt_frontend");

  return fe.FeMain(argc, argv);

  // while (! fe.ShutdownRequested()) {
  //   ::sleep(1);
  // }

  // fe.Disconnect();
  
  cm_disconnect_experiment();
  return 0;
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
