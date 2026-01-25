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

#include "utils/TEquipmentManager.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"

#include "node_frontend/TEqArtdaq.hh"
#include "node_frontend/TEqDisk.hh"
#include "node_frontend/TEqTrkDtc.hh"
#include "node_frontend/TEqCrvDtc.hh"

//-----------------------------------------------------------------------------
class NodeFrontend: public TMFrontend {
public:

  std::string           _host_label;
  // std::ofstream         _fout;
  std::streambuf*       _coutbuf;

  HNDLE                 _h_daq_host_conf;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  NodeFrontend();
  
  void HandleUsage() {}   //printf("FeEverything::HandleUsage!\n");  }
   
  TMFeResult HandleArguments(const std::vector<std::string>& args) {
    return TMFeOk();
  }
   
  virtual TMFeResult HandleFrontendInit(const std::vector<std::string>& args) override;
   
  TMFeResult HandleFrontendReady(const std::vector<std::string>& args) {
    TLOG(TLVL_DEBUG) << "-- START";
    //printf("FeEverything::HandleFrontendReady!\n");
    //FeStartPeriodicThread();
    //fMfe->StartRpcThread();
    TLOG(TLVL_DEBUG) << "-- END";
    return TMFeOk();
  }

  bool ShutdownRequested () { return fMfe->fShutdownRequested ; }
  void Disconnect        () { fMfe->Disconnect(); }
  void HandleFrontendExit() {} //printf("FeEverything::HandleFrontendExit!\n");  }
};


//-----------------------------------------------------------------------------
// HandleFrontendInit is called after connection to ODB
//-----------------------------------------------------------------------------
TMFeResult NodeFrontend::HandleFrontendInit(const std::vector<std::string>& args) {
  TLOG(TLVL_INFO) << "-- START";
//-----------------------------------------------------------------------------
// add eqm to the list of equipment pieces - it will be the only one 'TEquipment' thing
// managed by the frontend, TEquipment stores backward pointer to the frontend
// if this works, I can initialize frontend-specific equipment items here and simply
// add already initialize equipment to the equipment manager
//-----------------------------------------------------------------------------
  TEquipmentManager* eqm = new TEquipmentManager(_host_label.data(),__FILE__);
//-----------------------------------------------------------------------------
// expected types of equipment
// 1. DTC's
//-----------------------------------------------------------------------------
  OdbInterface* odb_i     = OdbInterface::Instance();

  _h_daq_host_conf        = odb_i->GetHostConfHandle(_host_label);

  HNDLE hdb               = odb_i->GetDbHandle();
  HNDLE h_active_run_conf = odb_i->GetActiveRunConfigHandle();
  HNDLE h_artdaq_conf     = odb_i->GetArtdaqConfHandle(h_active_run_conf,_host_label);

  HNDLE h_i;
  KEY   k_i;

  for (int i=0; db_enum_key(hdb,_h_daq_host_conf,i,&h_i) != DB_NO_MORE_SUBKEYS; i++) {
//-----------------------------------------------------------------------------
// skip 'Artdaq','Disk', etc folders
//-----------------------------------------------------------------------------
    db_get_key(hdb,h_i,&k_i);
    
    TLOG(TLVL_DEBUG) << "k[i].name:" << k_i.name;
    
    if (strstr(k_i.name,"DTC") != k_i.name)           continue;
//-----------------------------------------------------------------------------
// equipment names are capitalized, command names - not necessarily
// name is 'DTC0' or 'DTC1' , capitalize the subsystem name
// DTC name stub in ODB is also fully capitalized
//-----------------------------------------------------------------------------
    std::string subsystem = odb_i->GetString(h_i,"Subsystem");
    std::transform(subsystem.begin(),subsystem.end(),subsystem.begin(),::toupper);
    int dtc_enabled       = odb_i->GetEnabled(h_i);
    int pcie_addr         = odb_i->GetInteger(h_i,"PcieAddress");

    TLOG(TLVL_DEBUG) << std::format("subsystem:{} pcie_addr:{} enabled:{}",subsystem,pcie_addr,dtc_enabled);
   
    if (dtc_enabled) {
      std::string name = std::format("DTC{}",pcie_addr);
      TMu2eEqBase* eq;
      if      (subsystem == "CRV"    ) {
        eq = (TMu2eEqBase*) new TEqCrvDtc(name.data(),name.data(),h_active_run_conf,h_i);
      }
      else if (subsystem == "TRACKER") {
        eq = (TMu2eEqBase*) new TEqTrkDtc(name.data(),name.data(),h_active_run_conf,h_i);
      }
      eqm->AddEquipmentItem(eq);
      TLOG(TLVL_DEBUG) << std::format("subsystem:{} name:{} title:{}",eq->Subsystem(),eq->Name(),eq->Title());
    }
  }
//-----------------------------------------------------------------------------
// 2. ARTDAQ
//-----------------------------------------------------------------------------
  if (odb_i->GetEnabled( h_artdaq_conf)) { 
    TEqArtdaq* eq = new TEqArtdaq("ARTDAQ","Artdaq");
    eqm->AddEquipmentItem(eq);
  }
//-----------------------------------------------------------------------------
// 3. disk
//-----------------------------------------------------------------------------
  TEqDisk* eq_disk = new TEqDisk("DISK","Disk");
  eqm->AddEquipmentItem(eq_disk); // _eq_list.emplace_back(eq_disk);
  
  FeAddEquipment(eqm);

  TLOG(TLVL_INFO) << "-- END";
  return TMFeOk();
}
   
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
  _host_label = get_short_host_name(""); // local_subnet.data());
  FeSetName(_host_label.data());
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
