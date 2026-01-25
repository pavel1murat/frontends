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

#include "frontends/utils/utils.hh"
#include "frontends/utils/TEquipmentManager.hh"
#include "frontends/trk_cfg_frontend/TEqTracker.hh"

//-----------------------------------------------------------------------------
class TrackerCfgFrontend: public TMFrontend {
public:

  std::string           _name;
  std::string           _host_label;
  // std::ofstream         _fout;
  // std::streambuf*       _coutbuf;

  HNDLE                 _h_daq_host_conf;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TrackerCfgFrontend();
  
  void HandleUsage() {}   //printf("FeEverything::HandleUsage!\n");  }
  
  TMFeResult HandleArguments(const std::vector<std::string>& args) {
    //printf("FeEverything::HandleArguments!\n");
    return TMFeOk();
  }

  TMFeResult HandleFrontendInit(const std::vector<std::string>& args);
   
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
// TRK config frontend starts after the DTC frontends (500) and stops before them
//-----------------------------------------------------------------------------
TMFeResult TrackerCfgFrontend::HandleFrontendInit(const std::vector<std::string>& args) {
//-----------------------------------------------------------------------------
// add eqm to the list of equipment pieces - it will be the only one 'TEquipment' thing
// managed by the frontend, TEquipment stores backward pointer to the frontend
// if this works, I can initialize frontend-specific equipment items here and simply
// add already initialize equipment to the equipment manager
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm = new TEquipmentManager(_host_label.data(),__FILE__);

//-----------------------------------------------------------------------------
// expected types of equipment : 
// 1. tracker itself
//-----------------------------------------------------------------------------
  OdbInterface* odb_i = OdbInterface::Instance();

  _h_daq_host_conf = odb_i->GetHostConfHandle(_host_label);

  TEqTracker* eq = new TEqTracker("TRACKER","Tracker");

  eqm->AddEquipmentItem(eq);
  
  FeAddEquipment(eqm);

  cm_set_transition_sequence(TR_START,505);
  cm_set_transition_sequence(TR_STOP ,495);
  
  TLOG(TLVL_INFO) << "-- END";

  return TMFeOk();
}
   
//-----------------------------------------------------------------------------
// add future node frontend, 
// the base class constructor does nothing except instantiating the TFME thing
// but that doesn't connect to ODB yet...
// all parameters need to be initalized here
//-----------------------------------------------------------------------------
TrackerCfgFrontend::TrackerCfgFrontend() : TMFrontend() {

  TLOG(TLVL_DEBUG) << "-- START:";

  std::string hostname = get_short_host_name("");
 _name       = "trk_config";
 _host_label = get_short_host_name(""); // local_subnet.data());

 // can't access ODB yet at this point.. wait for a call to HandleFrontendInit
 // this is the name on a public network (w/o '-ctrl' and such) used as a label
 FeSetName(_name.data());
  
  TLOG(TLVL_DEBUG) << "-- END:";
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  signal(SIGPIPE, SIG_IGN);
  
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
