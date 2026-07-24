//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------

#include "iostream"
#include "vector"

#include "artdaq-core-mu2e/Overlays/Decoders/TrackerDataDecoder.hh"

#include "DtcInterfaceCrv.hh"
#include "otsdaq-mu2e-crv/FEInterfaces/ROC_Registers.h"
#include "TString.h"    // includes ROOT's Form

#include "TRACE/tracemf.h"
#define  TRACE_NAME "DtcInterfaceCrv"

using namespace DTCLib;
using namespace std;

namespace {
  bool initialized = 0;
};

//-----------------------------------------------------------------------------  
DtcInterfaceCrv::DtcInterfaceCrv(DTCLib::DTC* Dtc) : mu2edaq::DtcInterface(Dtc) {
  // initialization of the interface data members is done externally
  // nothing should happen here
}

//-----------------------------------------------------------------------------
// default ROC readout mode:0
//-----------------------------------------------------------------------------
DtcInterfaceCrv::DtcInterfaceCrv(int PcieAddr, uint LinkMask, bool SkipInit) 
  : mu2edaq::DtcInterface(PcieAddr, LinkMask, SkipInit) {
  if (not initialized) {
    initialized = true;
  }
};

//-----------------------------------------------------------------------------
// in many cases, want SkipInit=false
//-----------------------------------------------------------------------------
DtcInterfaceCrv* DtcInterfaceCrv::Instance(int PcieAddr, uint LinkMask, bool SkipInit) {
  TLOG(TLVL_DEBUG+1) << std::format("-- START");
  int pcie_addr = PcieAddr;
  if (pcie_addr < 0) {
//-----------------------------------------------------------------------------
// PCIE address is not specified, check environment
//-----------------------------------------------------------------------------
    if (getenv("DTCLIB_DTC") != nullptr) {
      pcie_addr = atoi(getenv("DTCLIB_DTC"));
    }
    else {
      TLOG(TLVL_ERROR) << Form("PcieAddr < 0 and $DTCLIB_DTC is not defined. BAIL out\n");
      return nullptr;
    }
  }
                                    
  TLOG(TLVL_DEBUG+1) << "inputs      : TRK PcieAddr: " << PcieAddr
                     << " pcie_addr:" << pcie_addr 
                     << " fgInstance[pcie_addr]:0x" << std::hex << fgInstance[pcie_addr] 
                     << " LinkMask:0x" << std::hex << LinkMask
                     << std::dec
                     << " SkipInit:" << SkipInit << std::endl;
    
  DtcInterfaceCrv* dtc_i (nullptr);
    
  if (fgInstance[pcie_addr] == nullptr) {
    fgInstance[pcie_addr] = new DtcInterfaceCrv(pcie_addr,LinkMask,SkipInit);
    dtc_i = (DtcInterfaceCrv*) fgInstance[pcie_addr];
    TLOG(TLVL_DEBUG+1) << "instantiated: TRK pcie_addr:" << pcie_addr
                       << " fgInstance[pcie_addr]:0x" << std::hex << dtc_i  
                       << " dtc_i->fLinkMask:0x" << std::hex << dtc_i->fLinkMask; 
  }
  else {
//-----------------------------------------------------------------------------
// already unutualized, double-check
//-----------------------------------------------------------------------------
    if (fgInstance[pcie_addr]->PcieAddr() != pcie_addr) {
      TLOG(TLVL_ERROR) << Form("DtcInterface::Instance already initialized with PcieAddress = %i. BAIL out\n", 
                               fgInstance[pcie_addr]->PcieAddr());
    }
    else {
      dtc_i = dynamic_cast<DtcInterfaceCrv*>(fgInstance[pcie_addr]);
      TLOG(TLVL_DEBUG+1) << "instantiated: TRK pcie_addr:" << pcie_addr
                         << " fgInstance[pcie_addr]:0x" << std::hex << dtc_i  
                         << " dtc_i->fLinkMask:0x" << std::hex << dtc_i->fLinkMask; 
    }
  }
  TLOG(TLVL_DEBUG+1) << std::format("-- END");
  return dtc_i;
}

//-----------------------------------------------------------------------------
// fRocReadoutMode is supposed to be already set, don't reinitialize
// fRocReadoutMode = 0: read ROC-renerated patterns, variable length
//                 = 1: read digis
//                 = 2: read ROC-generated patterns, fixed length
//
// this is fully tracker-specific
//-----------------------------------------------------------------------------
int DtcInterfaceCrv::InitRocReadoutMode(std::ostream& Stream) {
  int rc(0), tmo_ms(100);
  int rc_err(0x80000000);               // set bit 31 to make the number negative, but it is a bit code...
    
  TLOG(TLVL_DEBUG+1) << Form("-- START:");

  for (int lnk=0; lnk<6; lnk++) {
    if (not LinkEnabled(lnk))       continue;
    if (not LinkLocked (lnk)) {
      TLOG(TLVL_ERROR) << std::format("DTC:{} link:{} enabled but not locked",PcieAddr(),lnk);
      rc |= (1 << lnk);
      continue;
    }
    try {
      // 
      fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x009,false,tmo_ms);
      // usleep(100);
      fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x0A8,false,tmo_ms);
      // usleep(100);
      fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x1A8,false,tmo_ms);
      
      fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Register::GTP_CRC,1,false,tmo_ms);       // 1 --> r14: reset ROC
      std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCReset));
    }
    catch (...) {
      TLOG(TLVL_ERROR) << "Failed to reset link:" << lnk;
      rc |= (1 << (lnk+8));
    }
  }
                                        // so the return codes looked like are -201 etc and it would
                                        // be possible to identify the source
  if (rc != 0) rc = rc_err | rc;

  TLOG(TLVL_DEBUG) << std::format("-- END rc:0x{:08x}",rc);
  
  return rc;
}

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to R14 of each ROC specified as active by the mask
// by default, don't redefine the link mask
//-----------------------------------------------------------------------------
int DtcInterfaceCrv::ResetLink(int Link) {
  // int tmo_ms(100);
  int rc(0);
  
  TLOG(TLVL_DEBUG+1) << std::format("-- START: link:{} dont know what to do, just return",Link);
  return rc;
  

    // int lnk1(Link), lnk2(Link+1);
    // if (Link == -1) {
    //   lnk1 = 0;
    //   lnk2 = 6;
    // }
    // for (int lnk=lnk1; lnk<lnk2; ++lnk) {
    //   if (not LinkEnabled(lnk))       continue;
    //   if (not LinkLocked (lnk)) {
    //     TLOG(TLVL_ERROR) << std::format("DTC:{} link:{} enabled but not locked",PcieAddr(),lnk);
    //     rc += -10;
    //     continue;
    //   }
    //   try {
    //     fDtc->WriteROCRegister(DTC_Link_ID(lnk),registers::rocdcs::RESET_DDR,1,false,tmo_ms);       // 1 --> r14: reset ROC
    //     std::this_thread::sleep_for(std::chrono::microseconds(fSleepTimeROCReset));
    //   }
    //   catch (...) {
    //     TLOG(TLVL_ERROR) << "Failed to reset link:" << lnk;
    //     rc += -1;
    //   }
    // }
    
    // TLOG(TLVL_DEBUG+1) << std::format("-- END: rc:{}\n",rc);
    // return rc;
}

//-----------------------------------------------------------------------------
// Link = -1: all
//-----------------------------------------------------------------------------
int DtcInterfaceCrv::SetMarkerSync(int Link, bool enable) {
  int rc(0), tmo_ms(1000);
  
  int lnk_lo(Link), lnk_hi(Link+1);
  if (Link == -1) {
    lnk_lo=0;
    lnk_hi=6;
  }
  
  for (int lnk=lnk_lo; lnk<lnk_hi; lnk++) {
    uint32_t cr = fDtc->ReadROCRegister(DTC_Link_ID(lnk),ROC::CR,tmo_ms);
    cr          = enable ? (cr | (1u << 5)) : (cr & ~(1u << 5));
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::CR, cr, false, tmo_ms);
  }
  return rc;
}

//-----------------------------------------------------------------------------
// almost direct clone of SImon's code - to evaluate the amoung of porting effort
// Link = -1: all
//-----------------------------------------------------------------------------
int DtcInterfaceCrv::RocConfigure(int Link, bool gr, uint16_t grn, uint16_t uBoffset, uint16_t timeout, std::ostream& Stream) {
  int rc(0);
  int tmo_ms(1000);
  
  TLOG(TLVL_DEBUG+1) << "-- START";

  int lnk_lo(Link), lnk_hi(Link+1);
  if (Link == -1) {
    lnk_lo=0;
    lnk_hi=6;
  }
  for (int lnk=lnk_lo; lnk<lnk_hi; lnk++) {
    // set the ROC address
    fDtc->WriteROCRegister(DTC_Link_ID(lnk), ROC::ID, lnk, false, tmo_ms);
  
    // Enable the onboard PLL (1 is power down)
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::PLLStat, 0x0, false, tmo_ms);
    // and configure PLL mux to read digital lock
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::PLLMuxHigh, 0x12, false, tmo_ms);
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::PLLMuxHLow, 0x12, false, tmo_ms);
  
    // enable package forwarding based on markers
    // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::CR, 0x20);
    usleep(1000000);
    // return;
    SetMarkerSync(lnk,true);
  
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Clk80MHz, 0x1, false, tmo_ms);  // enable the 80MHz clock alignment
  
    // Reset procedure
    // Reset FM Rx: bit 0
    // Reset DDR write: bit 5 from 0 to 1
    // Reset DDR read (Init): bit 8
    // :::::::::::::::;
    // 0x009, 0x0A8, 0x1A8
  
    // Set CSR of data-FPGAs
    // bit 0: Reset FM Rx
    // bit 3: FM Rx Enable
    // bit 5: DDR Write Sequencer Enable
    // bit 7: DDR read sequencer Enable
    // bit 8: DDR Init (reset read pointer)
  
    // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x009);
    // ResetRxBuffers();
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x009, false, tmo_ms);
    // usleep(100);
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x0A8, false, tmo_ms);
    // usleep(100);
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x1A8, false, tmo_ms);
    // usleep(100);
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::GTP_CRC, 0x1, false, tmo_ms);

    // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0x08);  //
  
    // Reset input buffers
    // ResetRxBuffers();
    // usleep(100);
    //
    // Reset DDR on Data FPGAs
    // for(int i = 0; i < 3; ++i)
    //{
    //	fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[i] | ROC::Data_DDR_WriteHigh, 0x0);
    //	fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[i] | ROC::Data_DDR_WriteLow, 0x0);
    //	fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[i] | ROC::Data_DDR_ReadHigh, 0x0);
    //	fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[i] | ROC::Data_DDR_ReadLow, 0x0);
    //}
    // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data_Broadcast | ROC::Data_CRC, 0xA8);  //
  
    // Set TRIG 1
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::TRIG, 0x1, false, tmo_ms);
  
    // Enable GR package return
    TLOG(TLVL_DEBUG+1) << "Global Run Mode is " << (gr ? "enabled" : "disabled") << ".";

    if(gr)
      {
        fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::sendGR, 0x1 + (grn << 8), false, tmo_ms);
        // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::sendGR, 0x2);///
      
        // Disable send of active FEBs
        // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[0] | ROC::Data_LinkCtrl, 0x0);
        fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::uBOffset, 0x0, false, tmo_ms);
      }
    else
      {
        fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::sendGR, 0x0, false, tmo_ms);
      
        // Enable send of active FEBs
        // fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::Data[0] | ROC::Data_LinkCtrl, 0x0);
        fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::uBOffset, uBoffset, false, tmo_ms);
      }
  
    // 0xffff means disable timeout
    fDtc->WriteROCRegister(DTC_Link_ID(lnk),ROC::DRTimeout, timeout, false, tmo_ms);
  }

  TLOG(TLVL_DEBUG+1) << std::format("--END: rc:{}",rc);
  return rc;
}


//-----------------------------------------------------------------------------
int  DtcInterfaceCrv::Validate(ushort* Data, uint64_t EwTag, uint64_t* Offset, int PrintLevel, int* NErrRoc) {
  int nerr(0);
  return nerr;
}
