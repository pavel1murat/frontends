//-----------------------------------------------------------------------------
// iterate over the ODB subdirectory  and count the number of subkeys in it
//-----------------------------------------------------------------------------
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "midas.h"


//-----------------------------------------------------------------------------
int main(int argc, char **argv) {
  HNDLE hDB, hKey;
  INT   status, num_subkeys;
  KEY   key;

  cm_connect_experiment     (NULL, NULL, "Example", NULL);
  cm_get_experiment_database(&hDB, NULL);

  int rc(0);
  
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_set_thresholds");

  trkdaq::DtcInterface* dtc_i = DtcInterface::Instance(PcieAddr);

  int doit = o["Doit"] ;
  
  int lnk1 = Link;
  int lnk2 = Link+1;
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
    
    std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                          ConfName,_host_label.data(),PcieAddr,lnk);
    midas::odb o(panel_path);

    Stream << " -- link:" << lnk << std::endl;
    Stream << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
    Stream << "---------------------------------" << std::endl;

    for (int i=0; i<96; i++) {

      int ch_mask  = o["ch_mask"][i];
      int gain_cal = o["gain_cal"][i];
      int thr_cal  = o["threshold_cal"][i];
      int gain_hv  = o["gain_hv"][i];
      int thr_hv   = o["threshold_hv"][i];
      
      Stream << std::format("{:3d} {:3d}  {:3d} {:3d} {:3d} {:3d}\n",
                            i,ch_mask,gain_cal,gain_hv,thr_cal,thr_hv);

      if (doit != 0) {
        try {
          Dtc_i->ControlRoc_SetThreshold(Link,i,0,thr_cal );
          Dtc_i->ControlRoc_SetGain     (Link,i,0,gain_cal);
        
          Dtc_i->ControlRoc_SetThreshold(Link,i,1,thr_hv );
          Dtc_i->ControlRoc_SetGain     (Link,i,1,gain_hv);
        }
        catch(...) {
          Stream << "ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
          rc = -1;
          break;
        }
      }
    }
  }

  return rc;
 
  std::cout << "name:" <<  name << std::endl;

  // Disconnect from MIDAS and exit
  cm_disconnect_experiment();
  return 0;
}
