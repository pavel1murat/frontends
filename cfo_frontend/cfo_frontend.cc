///////////////////////////////////////////////////////////////////////////////
// P.Murat: CFO frontend , handles both emulated and external CFO
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
#define  TRACE_NAME "cfo_ext_frontend"

#include "utils/utils.hh"
#include "utils/TEquipmentManager.hh"
#include "cfo_frontend/TEqEmulatedCfo.hh"
#include "cfo_frontend/TEqExternalCfo.hh"

//-----------------------------------------------------------------------------
class CfoFrontend: public TMFrontend {
public:

  std::string            fName;
  std::ofstream         _fout;
  std::streambuf*       _coutbuf;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  CfoFrontend();
  
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
  void HandleFrontendExit() {} //printf("FeEverything::HandleFrontendExit!\n");  }
};

//-----------------------------------------------------------------------------
// the base class constructor does nothing except instantiating the TFME thing
// but that doesn't connect to ODB yet...
// all parameters need to be initalized here
//-----------------------------------------------------------------------------
CfoFrontend::CfoFrontend() : TMFrontend() {

  std::string hostname = get_short_host_name("");
  fName  = "cfo_fe";
  FeSetName(fName.data());
}

//-----------------------------------------------------------------------------
// at this point, the connection to ODB is already established
//-----------------------------------------------------------------------------
TMFeResult CfoFrontend::HandleFrontendInit(const std::vector<std::string>& args) {
  int        rc(0);
  TMFeResult res(TMFeOk());
  
  TLOG(TLVL_DEBUG) << std::format("-- START");
//-----------------------------------------------------------------------------
// add eqm to the list of equipment pieces - it will be the only one 'TEquipment' thing
// managed by the frontend, TEquipment stores backward pointer to the frontend
// if this works, I can initialize frontend-specific equipment items here and simply
// add already initialize equipment to the equipment manager
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm = new TEquipmentManager("cfo",__FILE__);
//-----------------------------------------------------------------------------
// expected equipment: emulated or 'external' CFO
//-----------------------------------------------------------------------------
  OdbInterface* odb_i     = OdbInterface::Instance();

  // _h_daq_host_conf        = odb_i->GetHostConfHandle(_host_label);

  HNDLE h_active_run_conf = odb_i->GetActiveRunConfigHandle();
  HNDLE h_cfo_conf        = odb_i->GetCfoConfHandle(h_active_run_conf);
  int emulated_mode       = odb_i->GetCfoEmulatedMode(h_cfo_conf);

  TMu2eEqBase* eq(nullptr);
  if (emulated_mode == 1) {
    eq = (TMu2eEqBase*) new TEqEmulatedCfo("CFO","CFO");
  }
  else if (emulated_mode == 0) {
    eq = (TMu2eEqBase*) new TEqExternalCfo("CFO","CFO");
  }
  else {
    std::string msg = std::format("undefined CFO emulated mode:{}. BAIL OUT",emulated_mode);
    TLOG(TLVL_ERROR) << msg;
    rc = -1;
    res = TMFeResult(rc,msg);
  }

  if (rc == 0) {
    eqm->AddEquipmentItem(eq);
    // add eq to the list of equipment pieces
    // equipment stores backward pointer to the frontend
    FeAddEquipment(eqm);
  }
    
  TLOG(TLVL_DEBUG) << std::format("-- END");
  return res;
}
   
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  signal(SIGPIPE, SIG_IGN);
  
  CfoFrontend fe;

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
