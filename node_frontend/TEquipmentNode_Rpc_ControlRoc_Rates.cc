/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"


int TEquipmentNode::Rpc_ControlRoc_Rates(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i, std::ostream& Stream, const char* ConfName) {
  // int timeout_ms(150);
  try {
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_rates");
    
    trkdaq::ControlRoc_Rates_t  par;
    // parameters should be taken from ODB - where from?
    
    par.num_lookback = o["num_lookback"];    //
    par.num_samples  = o["num_samples" ];    //
    int  print_level = o["PrintLevel"  ];
    
    if (o["UsePanelChannelMask"] != 0) {
//-----------------------------------------------------------------------------
// use straw mask defined for the panels, save the masks in the command ODB record
// the missing part - need to know the node name. But that is the local host name,
// the one the frontend is running on
// _read mask: 6 ushort's
//-----------------------------------------------------------------------------
      std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                            ConfName,_host_label.data(),PcieAddr,Link);
      HNDLE h_panel;
      int status = db_find_key(hDB, 0, panel_path.data() , &h_panel);
      // Stream << "h_panel:" <<  h_panel << " status:" << status << std::endl;
      
      HNDLE h_chmask;
      status = db_find_key(hDB, h_panel, "ch_mask" , &h_chmask);
      Stream << "h_chmask:" <<  h_chmask << " status:" << status << std::endl;
      
      int ch_mask[100];
      int  nbytes = 100*4;
      
      status = db_get_data(hDB, h_chmask, ch_mask, &nbytes, TID_INT32);
      // Stream << "nbytes:" <<  nbytes << " status:" << status << std::endl;
      
      // midas::odb   odb_panel(panel_path);
      for (int i=0; i<96; ++i) {
        //   int on_off = odb_panel["ch_mask"][i];
        int on_off = ch_mask[i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          par.ch_mask[iw] = 0;
        }
        // Stream << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
        par.ch_mask[iw] |= on_off << ib;
      }
      
      for (int i=0; i<6; i++) {
        // Stream << "i:" << i << " par.ch_mask[i]:0x" << std::hex << par.ch_mask[i] << std::endl;
        // o["ch_mask"][i] = par.ch_mask[i];
      }
    }
    else {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
//-----------------------------------------------------------------------------
      for (int i=0; i<6; i++) par.ch_mask[i] = o["ch_mask"][i];
    }
    
    Dtc_i->ControlRoc_Rates(Link,print_level,&par,Stream);
    
    printf("dtc_i->fLinkMask: 0x%04x\n",Dtc_i->fLinkMask);
  }
  catch(...) {
    Stream << "ERROR : coudn't execute ControlRoc_rates ... BAIL OUT" << std::endl;
  }
  
  return 0;
}
