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
#define  TRACE_NAME "node_frontend"

#include "node_frontend/TEquipmentManager.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

//-----------------------------------------------------------------------------
// static void usage() {
//    fprintf(stderr, "Usage: tmfe_example_mt ...\n");
//    exit(1);
// }

//-----------------------------------------------------------------------------
class NodeFrontend: public TMFrontend {
public:

  std::string            fName;
  std::ofstream         _fout;
  std::streambuf*       _coutbuf;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  NodeFrontend();
  
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
// add future node frontend, 
// the base class constructor does nothing except instantiating the TFME thing
// but that doesn't connect to ODB yet...
// all parameters need to be initalized here
//-----------------------------------------------------------------------------
NodeFrontend::NodeFrontend() : TMFrontend() {

  // OdbInterface* odb_i = OdbInterface::Instance();

  // HNDLE h_run_conf = odb_i->GetActiveRunConfigHandle();
  
  // std::string local_subnet = odb_i->GetString(h_run_conf,"DAQ/LocalSubnet");
  // this is the name on a public network (w/o '-ctrl' and such) used as a label
  std::string hostname = get_short_host_name(""); // local_subnet.data());
  fName  = hostname;
  FeSetName(fName.data());
  
  TEquipmentManager* eqm = new TEquipmentManager(hostname.data(),__FILE__);
//-----------------------------------------------------------------------------
// add eqm to the list of equipment pieces - it will be the only one 'TEquipment' thing
// managed by the frontend, TEquipment stores backward pointer to the frontend
//-----------------------------------------------------------------------------
  FeAddEquipment(eqm);
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

  NodeFrontend fe;

  // FeMain calls FeInit, and at that point connection to the experiment happens
  // this is too late for equipment to be initialized in the constructor
  // - however, after connecting, FeInit calls FeInitEquipments which loops
  //   over equipment pieces and calls EqInit function for each of them
  //   and after that enters the FeMainLoop
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
