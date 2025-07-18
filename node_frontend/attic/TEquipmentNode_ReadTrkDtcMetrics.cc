///////////////////////////////////////////////////////////////////////////////
#include "node_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"

#include <algorithm>
#include <cctype>

#include "midas.h"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"


//-----------------------------------------------------------------------------
void TEquipmentNode::ReadTrkDtcMetrics(int PcieAddr) {
//-----------------------------------------------------------------------------
// DTC temperature and voltages - for history 
//-----------------------------------------------------------------------------
  sprintf(text,"dtc%i",PcieAddr);

  try {
    std::vector<float> dtc_tv;
    for (const int reg : DtcRegHist) {
      uint32_t val;
      float    fval(-1.);
      try { 
        dtc_i->fDtc->GetDevice()->read_register(reg,100,&val); 
        if      ( reg == 0x9010) fval = (val/4096.)*503.975 - 273.15;   // temperature
        else                     fval = (val/4095.)*3.;                 // voltage
      }
      catch(...) {
        TLOG(TLVL_ERROR) << "failed to read register:" << reg;
        fval = -1;
      }
      
      dtc_tv.emplace_back(fval);
    }
    
    char buf[100];
    sprintf(buf,"dtc%i",idtc);
    
    midas::odb odb_dtc_tv = {{buf,{1.0f, 1.0f, 1.0f, 1.0f}}};
    odb_dtc_tv.connect(node_path+"/Variables");
        
    odb_dtc_tv[buf] = dtc_tv;
//-----------------------------------------------------------------------------
// non-history registers : 'dtr' = "DTcRegisters"
//-----------------------------------------------------------------------------
    ReadNonHistDtcRegisters(dtc_i);
//-----------------------------------------------------------------------------
// for each enabled DTC, loop over its ROCs and read ROC registers
// this part can depend on on the type of the ROC
// do it for the tracker
// don't use 'link' - ROOT doesn't like 'link' for a variable name
//-----------------------------------------------------------------------------
    auto trkdtc_i = dynamic_cast<trkdaq::DtcInterface*>(dtc_i);
    for (int ilink=0; ilink<6; ilink++) {
      if (trkdtc_i->LinkEnabled(ilink)) {
          
        if (_monitorRocRegisters) {
            
          std::vector<uint32_t>  roc_reg;
          roc_reg.reserve(RocRegisters.size());
            
          try {
            for (const int reg : RocRegisters) {
                // ROC registers store 16-bit words, don't know how to declare an array
                // of shorts for ODBXX, use uint32_t
              uint32_t dat = trkdtc_i->fDtc->ReadROCRegister(DTCLib::DTC_Link_ID(ilink),reg,100); 
              roc_reg.emplace_back(dat);
            }
              
            char buf[100];
            sprintf(buf,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
              
            midas::odb roc = {{"RegData",{1u}}};
            roc.connect(buf);
            roc["RegData"] = roc_reg;
          }
          catch (...) {
            TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " ROC:" << ilink << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
            // TODO
          }
        }
//-----------------------------------------------------------------------------
// SPI
//-----------------------------------------------------------------------------
        if (_monitorSPI) {
          TLOG(TLVL_DEBUG+1) << "saving ROC:" << ilink << " SPI data";
            
          struct trkdaq::TrkSpiData_t   spi;
          int rc = trkdtc_i->ControlRoc_ReadSpi_1(&spi,ilink,0);
          if (rc == 0) {
              
            std::vector<float> roc_spi;
            for (int iw=0; iw<trkdaq::TrkSpiDataNWords; iw++) {
              roc_spi.emplace_back(spi.Data(iw));
            }
              
            char buf[100];
            sprintf(buf,"rc%i%i",idtc,ilink);
            
            midas::odb xx = {{buf,{1.0f}}};
            xx.connect(node_path+"/Variables");
              
            xx[buf].resize(trkdaq::TrkSpiDataNWords);
            xx[buf] = roc_spi;
              
            TLOG(TLVL_DEBUG+1) << "ROC:" << ilink
                               << " saved N(SPI) words:" << trkdaq::TrkSpiDataNWords;
          }
          else {
            TLOG(TLVL_ERROR)   << "failed to read SPI, DTC:" << idtc << " ROC:" << ilink;
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
            // TODO
          }
        }
//-----------------------------------------------------------------------------
// ROC rates
// for now, assume that the clock has been set to internal ,
// need to find the right place to set marker_clock to 0 (and may be recover in the end),
// will do it right later 
//-----------------------------------------------------------------------------
        if (_monitorRates and (transition_in_progress == 0) and (running_state != STATE_RUNNING)) {
          TLOG(TLVL_DEBUG+1) << "MONITOR RATES link:" << ilink;
//-----------------------------------------------------------------------------
// for monitoring, want to read ALL channels.
// 1. run read command enabling the internal clock and setting the read mask to read all channels
// 2. run rates command with all channels enabled
// 3. run read command restoring the clock marker (source of the clock) and the read mask
//    which someone may rely on
//-----------------------------------------------------------------------------
          midas::odb o_read_cmd   ("/Mu2e/Commands/Tracker/DTC/control_ROC_read");

          trkdaq::ControlRoc_Read_Input_t0 pread;                // ch_mask is set to all oxffff
                                        // save the read command ch_mask
          uint16_t saved_ch_mask[6];
          for (int i=0; i<6; ++i) saved_ch_mask[i] = o_read_cmd["ch_mask"][i];
            
          pread.adc_mode        = o_read_cmd["adc_mode"     ];   // -a
          pread.tdc_mode        = o_read_cmd["tdc_mode"     ];   // -t 
          pread.num_lookback    = o_read_cmd["num_lookback" ];   // -l 
          
          pread.num_samples     = o_read_cmd["num_samples"  ];   // -s
          pread.num_triggers[0] = o_read_cmd["num_triggers"][0]; // -T 10
          pread.num_triggers[1] = o_read_cmd["num_triggers"][1]; //
//-----------------------------------------------------------------------------
// this is a tricky place: rely on that the READ command ODB record
// stores the -p value used during the data taking
//-----------------------------------------------------------------------------
          pread.enable_pulser   = o_read_cmd["enable_pulser"];   // -p 1
          pread.marker_clock    = 0;                             // to read the rates, enable internal clock
          pread.mode            = o_read_cmd["mode"         ];   // 
          pread.clock           = o_read_cmd["clock"        ];   //
          
          int print_level       = 0;
            
          trkdtc_i->ControlRoc_Read(&pread,ilink,print_level);
               
          std::vector<uint16_t> rates;
          trkdaq::ControlRoc_Rates_t* par(nullptr); // defaults are OK - read all channels
          int rc = trkdtc_i->ControlRoc_Rates(ilink,&rates,print_level,par,nullptr);
//-----------------------------------------------------------------------------
// and restore the READ command mask and the clock
//-----------------------------------------------------------------------------
          pread.marker_clock    = o_read_cmd["marker_clock" ];   // restore the marker_clock mode
          for (int i=0; i<6; ++i) pread.ch_mask[i] = saved_ch_mask[i];
          trkdtc_i->ControlRoc_Read(&pread,ilink,print_level);
//-----------------------------------------------------------------------------
// print diagnostics
//-----------------------------------------------------------------------------
          if (rc == 0) {
            char buf[16];
            sprintf(buf,"rr%i%i",idtc,ilink);
              
            midas::odb vars(node_path+"/Variables");
            vars[buf] = rates;
            
            TLOG(TLVL_DEBUG+1) << "ROC:" << ilink << " saved rates to \"" << node_path+"/Variables[" << buf << "\"], nw:" << rates.size();
          }
          else {
            TLOG(TLVL_ERROR) << "failed to read rates DTC:" << idtc << " ROC:" << ilink;
//-----------------------------------------------------------------------------
// set ROC status to -1
//-----------------------------------------------------------------------------
            // TODO
          }
        }
      }
    }
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "failed to read DTC:" << idtc << " registers";
//-----------------------------------------------------------------------------
// set DTC status to -1
//-----------------------------------------------------------------------------
    // TODO
  }
  return;
}
