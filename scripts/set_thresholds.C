//-----------------------------------------------------------------------------
// for this to work, need a ROOT interface to OBD++
//-----------------------------------------------------------------------------
#define __CLING__ 1

#include "iostream"
#include "nlohmann/json.hpp"

#include "TH1.h"
#include "TStopwatch.h"

#include "dtcInterfaceLib/DTC.h"
#include "cfoInterfaceLib/CFO.h"
#include "cfoInterfaceLib/CFO_Compiler.hh"

using namespace CFOLib;
using namespace DTCLib;

// #include "print_buffer.C"

#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
#include "midas.h"
#include "odbxx.h"


trkdaq::DtcInterface* dtc_i;

//-----------------------------------------------------------------------------
int set_thresholds(const char* PanelName, int Plane, int Panel, int Doit=0) {

  std::string panel_path = std::format("/Mu2e/ActiveRunConfiguration/Tracker/Station_00/Plane_0{}/Panel_0{}",
                                       Plane,Panel);
  midas::odb o(panel_path);

  // for now: Plane = dtc_id
                                                                                   
  // auto dtc_i = trkdaq::DtcInterface::Instance(Plane);
  
  printf("------------------- setting thresholds for plane=%i Panel=%i\n",Plane, Panel);

  for (int i=0; i<96; i++) {

    int gain_cal = o["gain_cal"][i];
    int thr_cal  = o["threshold_cal"][i];
    int gain_hv  = o["gain_hv"][i];
    int thr_hv   = o["threshold_hv"][i];

    printf("i:%i gain_cal:%3i gain_hv:%3i thr_cal:%3i thr_hv:%i\n",
           i,gain_cal,gain_hv,thr_cal,thr_hv);

    if (Doit != 0) {
      dtc_i->ControlRoc_SetThreshold(Panel,i,0,thr_cal );
      dtc_i->ControlRoc_SetGain     (Panel,i,0,gain_cal);

      dtc_i->ControlRoc_SetThreshold(Panel,i,1,thr_hv );
      dtc_i->ControlRoc_SetGain     (Panel,i,1,gain_hv);
    }
  }
  
  return 0;
}


//-----------------------------------------------------------------------------
// for now, Plane= DTC PCIE_ADDR, panel: Link
//-----------------------------------------------------------------------------
int set_thresholds_new(const char* PanelName, int Plane, int Panel, int Doit=0) {

  std::string panel_path = std::format("/Mu2e/ActiveRunConfiguration/Tracker/Station_00/Plane_0{}/Panel_0{}",
                                       Plane,Panel);
  midas::odb o(panel_path);

  uint16_t data[4*96];

  int pcie_addr = Plane;
  int lnk       = Panel; // to be combed later
  
  printf("------------------- setting thresholds for plane=%i Panel=%i\n",Plane, Panel);
//-----------------------------------------------------------------------------
// read threshodls/gains from ODB
//-----------------------------------------------------------------------------
  for (int i=0; i<96; i++) {

    data[i+96*0] = (uint16_t) o["gain_cal"     ][i];
    data[i+96*1] = (uint16_t) o["gain_hv"      ][i];
    data[i+96*2] = (uint16_t) o["threshold_cal"][i];
    data[i+96*3] = (uint16_t) o["threshold_hv" ][i];

    printf("i:%i gain_cal:%5i gain_hv:%5i thr_cal:%5i thr_hv:%5i\n",
           i,data[i+96*0],data[i+96*1],data[i+96*2],data[i+96*3]);
  }

  if (Doit != 0) {
    dtc_i->ControlRoc_SetThresholds(lnk,data,0x11);
  }
  
  return 0;
}


//-----------------------------------------------------------------------------
int set_all_thresholds(int Station = 0,int Doit = 0) {

  cm_connect_experiment(NULL, NULL, "test_025", NULL);
  
  dtc_i = trkdaq::DtcInterface::Instance(0,0x111111,false);

  set_thresholds("MN261",0,0,Doit);
  set_thresholds("MN248",0,1,Doit);
  set_thresholds("MN224",0,2,Doit);
  set_thresholds("MN262",0,3,Doit);
  set_thresholds("MN273",0,4,Doit);
  set_thresholds("MN276",0,5,Doit);

  dtc_i = trkdaq::DtcInterface::Instance(1,0x111111,false);
  set_thresholds("MN253",1,0,Doit);
  set_thresholds("MN101",1,1,Doit);
  set_thresholds("MN219",1,2,Doit);
  set_thresholds("MN213",1,3,Doit);
  set_thresholds("MN235",1,4,Doit);
  set_thresholds("MN247",1,5,Doit);

  cm_disconnect_experiment();
  return 0;
}
