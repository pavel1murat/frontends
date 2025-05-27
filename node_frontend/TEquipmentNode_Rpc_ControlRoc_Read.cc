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

//-----------------------------------------------------------------------------
int TEquipmentNode::Rpc_ControlRoc_Read(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i,
                                        std::ostream& Stream, const char* ConfName) {
  
  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read");

  trkdaq::ControlRoc_Read_Input_t0 par;

  par.adc_mode        = o["adc_mode"     ];   // -a
  par.tdc_mode        = o["tdc_mode"     ];   // -t 
  par.num_lookback    = o["num_lookback" ];   // -l 
  
  par.num_samples     = o["num_samples"  ];   // -s
  par.num_triggers[0] = o["num_triggers"][0]; // -T 10
  par.num_triggers[1] = o["num_triggers"][1]; //


  // this is how we get the panel name

  int use_panel_channel_mask = o["use_panel_channel_mask"];
  int print_level            = o["print_level"];
    
  Stream << "use_panel_channel_mask:" <<  use_panel_channel_mask << std::endl;

  int lnk1(Link), lnk2(Link+1);
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (use_panel_channel_mask == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
//-----------------------------------------------------------------------------
      for (int i=0; i<6; i++) par.ch_mask[i] = o["ch_mask"][i];
    }
    else {
//-----------------------------------------------------------------------------
// use straw mask defined for the panels, save the masks in the command ODB record
// the missing part - need to know the node name. But that is the local host name,
// the one the frontend is running on
// _read mask: 6 ushort's
//-----------------------------------------------------------------------------
      std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                            ConfName,_host_label.data(),PcieAddr,lnk);
      midas::odb   odb_panel(panel_path);
      for (int i=0; i<96; ++i) {
        int on_off = odb_panel["ch_mask"][i];
        // int on_off = ch_mask[i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          par.ch_mask[iw] = 0;
        }
        // Stream << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
        par.ch_mask[iw] |= on_off << ib;
      }

      if (print_level > 0) {
        Stream << "par.ch_mask:";
        for (int i=0; i<6; i++) {
          Stream << " " << std::hex << par.ch_mask[i];
        }
        Stream << std::endl;
      }
    }
        
  // for (int i=0; i<6; i++) par.ch_mask[i] = o["ch_mask"][i];
    
    par.enable_pulser   = o["enable_pulser"];   // -p 1
    par.marker_clock    = o["marker_clock" ];   // -m 3 (for data taking) , 0: for noise
    par.mode            = o["mode"         ];   // 
    par.clock           = o["clock"        ];   //
  
    try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
      Dtc_i->ControlRoc_Read(&par,lnk,print_level,Stream);
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_Read for link:" << lnk << " ... BAIL OUT" << std::endl;
    }
  }
  return 0;
}
