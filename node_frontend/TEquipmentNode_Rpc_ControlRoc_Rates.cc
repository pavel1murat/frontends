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

//-----------------------------------------------------------------------------
// takes parameters frpm ODB
//-----------------------------------------------------------------------------
int TEquipmentNode::Rpc_ControlRoc_Rates(int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i, std::ostream& Stream, const char* ConfName) {
  // int timeout_ms(150);

  std::vector<uint16_t> rates  [6];   // 6 ROCs max
  std::vector<int>      ch_mask[6];   // used for printing only

  uint16_t              rates_ch_mask[6]; // cached masks from the RATES command ODB record

  TLOG(TLVL_DEBUG) << "--- START";
  
  midas::odb o_cmd("/Mu2e/Commands/Tracker/DTC/control_ROC_rates");
    
  trkdaq::ControlRoc_Rates_t  prates;
    
  prates.num_lookback = o_cmd["num_lookback"];    //
  prates.num_samples  = o_cmd["num_samples" ];    //

  int  print_level = o_cmd["print_level" ];

  for (int i=0; i<6; i++) {
    rates  [i].reserve(96);
    ch_mask[i].reserve(96);
  }

  int lnk1(Link), lnk2(Link+1);
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  TLOG(TLVL_DEBUG) << "--- 002 lnk1:" << lnk1 << " lnk2:" << lnk2;
  
  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    if (Dtc_i->LinkEnabled(lnk) == 0) continue ;
    
    if (o_cmd["UsePanelChannelMask"] == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
// '6' below is a random coincidence
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; ++iw) {
        uint16_t w = o_cmd["ch_mask"][iw];
        // prates.ch_mask[iw] = w;
        rates_ch_mask [iw] = w;         // cache it here 

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
// use straw mask defined by the panel // save the masks in the command ODB record <=== what do I need it for ??? probably this is not needed!
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
          rates_ch_mask[iw] = 0;
        }
        rates_ch_mask[iw] |= (on_off << ib);
        // Stream << "ch_mask["<<i<<"]:" << on_off << " iw:" << iw << " ib:" << ib << std::endl;

        ch_mask[lnk].push_back(on_off);
      }
                                        // update RATES command channel mask in ODB
      // for (int iw=0; iw<6; iw++) {
      //   o_cmd["ch_mask"][iw] = prates.ch_mask[iw];
      // }
    }
//-----------------------------------------------------------------------------
// print the mask
//-----------------------------------------------------------------------------
    if (print_level&0x8) {
      Stream << "par.ch_mask:";
      for (int iw=0; iw<6; iw++) {
        Stream << " " << std::hex << rates_ch_mask[iw];
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

      midas::odb o_read_cmd   ("/Mu2e/Commands/Tracker/DTC/control_ROC_read");

      trkdaq::ControlRoc_Read_Input_t0 pread;

      pread.adc_mode        = o_read_cmd["adc_mode"     ];   // -a
      pread.tdc_mode        = o_read_cmd["tdc_mode"     ];   // -t 
      pread.num_lookback    = o_read_cmd["num_lookback" ];   // -l 
  
      pread.num_samples     = o_read_cmd["num_samples"  ];   // -s
      pread.num_triggers[0] = o_read_cmd["num_triggers"][0]; // -T 10
      pread.num_triggers[1] = o_read_cmd["num_triggers"][1]; //
//-----------------------------------------------------------------------------
// when reading RATES, always read all channels, no matter what the current settings are
// and the reasonable settings could be 1) ALL CHANNELS 2) a channel mask is defined by the panel
// rely on the RATES command to have that setting right - defined by the panel
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; iw++) pread.ch_mask[iw] = 0XFFFF; // prates.ch_mask[iw];
//-----------------------------------------------------------------------------
// this is a tricky place: rely on that the READ command ODB record
// stores the -p value used during the data taking
//-----------------------------------------------------------------------------
      pread.enable_pulser   = o_read_cmd["enable_pulser"];   // -p 1
      pread.marker_clock    = 0;                             // to read the rates, enable internal clock
      pread.mode            = o_read_cmd["mode"         ];   // 
      pread.clock           = o_read_cmd["clock"        ];   //

      if (print_level & 0x8) {
        Stream <<  "--- running control_roc_read marker_clock:" << pread.marker_clock
               << " enable_pulser:" << pread.enable_pulser << std::endl;
      }

      Dtc_i->ControlRoc_Read(&pread,lnk,print_level,Stream);
      
      if (print_level & 0x8) Stream <<  "--- running control_roc_rates" << std::endl;

      for (int iw=0; iw<6; iw++) prates.ch_mask[iw] = 0XFFFF; // want to read all channels, RATES doesn't change any masks

      Dtc_i->ControlRoc_Rates(lnk,&rates[lnk],print_level,&prates,&Stream);

      pread.marker_clock    = o_read_cmd["marker_clock" ];   // recover marker_clock mode

      if (print_level & 0x8) {
        Stream <<  "--- running control_roc_read marker_clock:" << pread.marker_clock
               << " enable_pulser:" << pread.enable_pulser << std::endl;
      }
//-----------------------------------------------------------------------------
// restore the channel mask relying on the RATES command parameters to define it right:
// either all channels are enabled, or the channel mask is defined by the panel
//-----------------------------------------------------------------------------
      for (int iw=0; iw<6; iw++) pread.ch_mask[iw] = rates_ch_mask[iw];
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
  
  TLOG(TLVL_DEBUG) << "--- END";
  return 0;
}
