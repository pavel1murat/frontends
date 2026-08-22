//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
//-----------------------------------------------------------------------------

#include "iostream"
#include "vector"

#include "DtcInterfaceCal.hh"
// #include "otsdaq-mu2e-cal/FEInterfaces/ROC_Registers.h"
#include "TString.h"        // includes ROOT's Form

#include "TRACE/tracemf.h"
#define  TRACE_NAME "DtcInterfaceCal"

using namespace DTCLib;
using namespace std;

namespace {
  bool initialized = 0;
};

//-----------------------------------------------------------------------------  
DtcInterfaceCal::DtcInterfaceCal(DTCLib::DTC* Dtc) : mu2edaq::DtcInterfaceBase(Dtc) {
  // initialization of the interface data members is done externally
  // nothing should happen here
}

//-----------------------------------------------------------------------------
// default ROC readout mode:0
//-----------------------------------------------------------------------------
DtcInterfaceCal::DtcInterfaceCal(int PcieAddr, uint LinkMask, bool SkipInit) 
  : mu2edaq::DtcInterfaceBase(PcieAddr, LinkMask, SkipInit) {
  if (not initialized) {
    initialized = true;
  }
};

//-----------------------------------------------------------------------------
// in many cases, want SkipInit=false
//-----------------------------------------------------------------------------
DtcInterfaceCal* DtcInterfaceCal::Instance(int PcieAddr, uint LinkMask, bool SkipInit) {
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
    
  DtcInterfaceCal* dtc_i (nullptr);
    
  if (fgInstance[pcie_addr] == nullptr) {
    fgInstance[pcie_addr] = new DtcInterfaceCal(pcie_addr,LinkMask,SkipInit);
    dtc_i = (DtcInterfaceCal*) fgInstance[pcie_addr];
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
      dtc_i = dynamic_cast<DtcInterfaceCal*>(fgInstance[pcie_addr]);
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
int DtcInterfaceCal::InitRocReadoutMode(std::ostream& Stream) {
  int rc(0), tmo_ms(100);
  int rc_err(0x80000000);               // set bit 31 to make the number negative, but it is a bit code...
    
  TLOG(TLVL_DEBUG+1) << std::format("-- START: dont know wahat to do, do nothing");

  TLOG(TLVL_DEBUG) << std::format("-- END rc:0x{:08x}",rc);
  
  return rc;
}

//-----------------------------------------------------------------------------
// ROC reset : write 0x1 to R14 of each ROC specified as active by the mask
// by default, don't redefine the link mask
//-----------------------------------------------------------------------------
int DtcInterfaceCal::ResetLink(int Link) {
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
int  DtcInterfaceCal::Validate(ushort* Data, uint64_t EwTag, uint64_t* Offset, int PrintLevel, int* NErrRoc) {
  int nerr(0);
  return nerr;
}
