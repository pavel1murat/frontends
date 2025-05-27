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

#include "TRACE/tracemf.h"


int TEquipmentNode::Rpc_ControlRoc_Rates(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i, std::ostream& Stream, const char* ConfName) {
  // int timeout_ms(150);

  std::vector<uint16_t> rates  [6];   // 6 ROCs max
  std::vector<int>      ch_mask[6];

  TLOG(TLVL_INFO) << "--- START";
  
  midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_rates");
    
  trkdaq::ControlRoc_Rates_t  par;
    
  par.num_lookback = o["num_lookback"];    //
  par.num_samples  = o["num_samples" ];    //

  int  print_level = o["print_level"  ];

  for (int i=0; i<6; i++) {
    rates  [i].reserve(96);
    ch_mask[i].reserve(96);
  }

  int lnk1(Link), lnk2(Link+1);
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_INFO) << "--- 002";
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (Dtc_i->LinkEnabled(lnk) == 0) continue ;
    
    if (o["UsePanelChannelMask"] == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
// '6' below is a random coincidence
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; ++iw) {
        uint16_t w = o["ch_mask"][iw];
        par.ch_mask[iw] = w;

        for (int k=0; k<16; ++k) {
          // int loc = iw*16+k;
          int flag = (w >> k) & 0x1;
          // Stream << "mask loc:" << loc << " flag:" << flag <<std::endl;
          ch_mask[lnk].push_back(flag);
        }
      }
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
      std::string  panel_name = odb_panel["Name"];

      if (print_level&0x8) Stream << "panel_path:" << panel_path << " panel_name:" << panel_name << std::endl;

      for (int i=0; i<96; ++i) {
        // int on_off = ch_mask_odb[i];
        int on_off = odb_panel["ch_mask"][i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          par.ch_mask[iw] = 0;
        }
        par.ch_mask[iw] |= (on_off << ib);
        // Stream << "ch_mask["<<i<<"]:" << on_off << " iw:" << iw << " ib:" << ib << std::endl;

        ch_mask[lnk].push_back(on_off);
      }
                                        // update panel mask
      for (int iw=0; iw<6; iw++) {
        o["ch_mask"][iw] = par.ch_mask[iw];
      }
    }
//-----------------------------------------------------------------------------
// print the mask
//-----------------------------------------------------------------------------
    if (print_level&0x8) {
      Stream << "par.ch_mask:";
      for (int iw=0; iw<6; iw++) {
        Stream << " " << std::hex << par.ch_mask[iw];
      }
      Stream << std::endl;
    }

    try {
//-----------------------------------------------------------------------------
// switch to internal clock to measure the rates
// assume that, with the exception of the clock_mode, which we flip, the ODB has the right set
// of parameters stored
//-----------------------------------------------------------------------------
      if (print_level & 0x8) {
        Stream <<  std::format("dtc_i->fLinkMask: 0x%04x",Dtc_i->fLinkMask) << std::endl;
      }

      midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_read");

      trkdaq::ControlRoc_Read_Input_t0 pread;

      pread.adc_mode        = o["adc_mode"     ];   // -a
      pread.tdc_mode        = o["tdc_mode"     ];   // -t 
      pread.num_lookback    = o["num_lookback" ];   // -l 
  
      pread.num_samples     = o["num_samples"  ];   // -s
      pread.num_triggers[0] = o["num_triggers"][0]; // -T 10
      pread.num_triggers[1] = o["num_triggers"][1]; //

      // the mask is the same

      for (int iw=0; iw<6; iw++) pread.ch_mask[iw] = par.ch_mask[iw];
      
      pread.enable_pulser   = o["enable_pulser"];   // -p 1
      pread.marker_clock    = 0;                    // to read the rates 
      pread.mode            = o["mode"         ];   // 
      pread.clock           = o["clock"        ];   //

      Dtc_i->ControlRoc_Read(&pread,lnk,print_level,Stream);
      Dtc_i->ControlRoc_Rates(lnk,&rates[lnk],print_level,&par,Stream);

      pread.marker_clock    = o["marker_clock" ];   // recover marker_clock mode
      Dtc_i->ControlRoc_Read(&pread,lnk,print_level,Stream);

      if (print_level & 0x2) { // detailed printout, one ROC only
        Dtc_i->PrintRatesSingleRoc(&rates[lnk],&ch_mask[lnk],Stream);
      }
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_rates for link:" << lnk << "... BAIL OUT";
      return -1;
    }
  }
//-----------------------------------------------------------------------------
// if we got here, the execution succeeded, print
//-----------------------------------------------------------------------------
  if (print_level & 0x4) {
    Dtc_i->PrintRatesAllRocs(rates,ch_mask,Stream);
  }
  
  TLOG(TLVL_INFO) << "--- END";
  return 0;
}
