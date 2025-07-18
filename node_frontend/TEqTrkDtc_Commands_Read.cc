/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

//-----------------------------------------------------------------------------
int TEqTrkDtc::Read(std::ostream& Stream) {
  
  OdbInterface* odb_i      = OdbInterface::Instance();
  HNDLE         h_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   conf_name  = odb_i->GetRunConfigName(h_run_conf);

  HNDLE         h_cmd     = odb_i->GetDtcCommandHandle(_host_label,_dtc_i->PcieAddr());
  HNDLE         h_cmd_par = odb_i->GetHandle(h_cmd,"read"); // for now

  trkdaq::ControlRoc_Read_Input_t0 par;

  par.adc_mode        = odb_i->GetUInt16(h_cmd_par,"adc_mode"     );   // -a
  par.tdc_mode        = odb_i->GetUInt16(h_cmd_par,"tdc_mode"     );   // -t 
  par.num_lookback    = odb_i->GetUInt16(h_cmd_par,"num_lookback" );   // -l 
  
  par.num_samples     = odb_i->GetUInt16(h_cmd_par,"num_samples"    ); // -s
  par.num_triggers[0] = odb_i->GetUInt16(h_cmd_par,"num_triggers[0]"); // -T 10
  par.num_triggers[1] = odb_i->GetUInt16(h_cmd_par,"num_triggers[1]"); //

  // this is how we get the panel name

  int use_panel_channel_mask = odb_i->GetInteger(h_cmd_par,"use_panel_channel_mask");
  int print_level            = odb_i->GetInteger(h_cmd_par,"print_level");
    
  // Stream << "use_panel_channel_mask:" <<  use_panel_channel_mask << std::endl;

  int lnk1 = odb_i->GetInteger(h_cmd_par,"link");
  int lnk2 = lnk1+1;
  if (lnk1 == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }

  for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    Stream << "----------------- link:" << lnk;
    if (_dtc_i->LinkEnabled(lnk) == 0) {
      if (print_level != 0) {
        Stream << " is disabled" << std::endl;
        continue;
      }
    }
    if (use_panel_channel_mask == 0) {
//-----------------------------------------------------------------------------
// use masks stored in the command ODB record
//-----------------------------------------------------------------------------
      odb_i->GetArray(h_cmd_par,"ch_mask",TID_WORD,par.ch_mask,6);
    }
    else {
//-----------------------------------------------------------------------------
// use straw mask defined for the panels, save the masks in the command ODB record
// the missing part - need to know the node name. But that is the local host name,
// the one the frontend is running on
// _read mask: 6 ushort's
//-----------------------------------------------------------------------------
      std::string  panel_path = std::format("/Mu2e/RunConfigurations/{:s}/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                            conf_name.data(),_host_label.data(),_dtc_i->PcieAddr(),lnk);
      midas::odb   odb_panel(panel_path);
      for (int i=0; i<96; ++i) {
        int on_off = odb_panel["ch_mask"][i];
        int iw = i / 16;
        int ib = i % 16;
        if (ib == 0) {
          par.ch_mask[iw] = 0;
        }
        // Stream << "ch_mask["<<i<<"]:" << ch_mask[i] << " iw:" << iw << " ib:" << ib << std::endl;; 
        par.ch_mask[iw] |= on_off << ib;
      }

      if (print_level & 0x4) {
        Stream << "par.ch_mask:";
        for (int i=0; i<6; i++) {
          Stream << " " << std::hex << par.ch_mask[i];
        }
        // Stream << std::endl;
      }
    }
        
    par.enable_pulser   = odb_i->GetUInt16(h_cmd_par,"enable_pulser");   // -p 1
    par.marker_clock    = odb_i->GetUInt16(h_cmd_par,"marker_clock" );   // -m 3: data taking (ext clock), -m 0: rates (int clock)
    par.mode            = odb_i->GetUInt16(h_cmd_par,"mode"         );   // not used ?
    par.clock           = odb_i->GetUInt16(h_cmd_par,"clock"        );   // 
  
    try {
//-----------------------------------------------------------------------------
// ControlRoc_Read handles roc=-1 internally
//-----------------------------------------------------------------------------
      _dtc_i->ControlRoc_Read(&par,lnk,print_level,Stream);
      Stream << " SUCCESS" << std::endl;
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_Read for link:" << lnk << " ... BAIL OUT" << std::endl;
    }
  }
  return 0;
}
