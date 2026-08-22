///////////////////////////////////////////////////////////////////////////////
// separate print routines
///////////////////////////////////////////////////////////////////////////////
#include "iostream"
#include "vector"
#include "DtcInterfaceCal.hh"
#include "TString.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "DtcInterfaceCal"

using namespace DTCLib;
using namespace std;

// #include "otsdaq-mu2e-cal/FEInterfaces/ROC_Registers.h"

//-----------------------------------------------------------------------------
// print value of the register Reg, for multiple ROCs
//-----------------------------------------------------------------------------
void DtcInterfaceCal::PrintRocRegister(uint32_t Reg, std::string& Desc, int Format, int LinkMask,std::ostream& Stream) {
  TLOG(TLVL_DEBUG+1) << std::format("-- START: Reg:{} Format:{} LinkMask:0x{:08x}",Reg,Format,LinkMask);
    
  std::string text;
  for (int i=0; i<6; i++) {
    int used = (LinkMask >> 4*i) & 0x1;
    if (used == 0)                                        continue;
    // need this if accidentally called directly
    if (not LinkLocked(i)) {
      TLOG(TLVL_ERROR) << std::format("link:{} enabled but not locked",i);
      continue;
    }
    
    DTC_Link_ID link = DTC_Link_ID(i);
    uint32_t dat;
    
    dat = fDtc->ReadROCRegister(link,Reg,100);
      text += Form("     0x%04x",dat);
  }
  std::string sreg = Form("reg(0x%02x)    ",Reg);
  
  if (Format == 1) text += Form(" %s",Desc.data());
  Stream << Form("%-18s     %s\n",sreg.data(),text.data());
  
  TLOG(TLVL_DEBUG+1) << std::format("-- END");
}

//-----------------------------------------------------------------------------
// print 32-bit word made out of two ROC registers, Reg (lo) and Reg+1 (hi)
//-----------------------------------------------------------------------------
void DtcInterfaceCal::PrintRocRegister2(uint RegLo, uint RegHi, std::string& Desc, int Format, int LinkMask, std::ostream& Stream) {

  TLOG(TLVL_DEBUG+1) << std::format("-- START: RegLo:0x{:04x} RegHi:0x{:04x} Format:{} LinkMask:0x{:08x}",
                                    RegLo,RegHi,Format,LinkMask);
  
  std::string text;
  for (int i=0; i<6; i++) {
    int used = (LinkMask >> 4*i) & 0x1;
    if (used == 0)                                        continue;
    // need this if accidentally called directly
    if (not LinkLocked(i)) {
      TLOG(TLVL_ERROR) << std::format("link:{} enabled but not locked",i);
      continue;
    }
    
    DTC_Link_ID link = DTC_Link_ID(i);
    uint32_t iw1, iw2, iw;
    
    iw1 = fDtc->ReadROCRegister(link,RegLo,100);
    iw2 = fDtc->ReadROCRegister(link,RegHi,100);
    iw  = (iw2 << 16) | iw1;
    text += Form(" 0x%08x",iw);
  }
  
  if (Format == 1) text += Form(" %s",Desc.data());
  
  std::string sreg = Form("reg(0x%02x)<<16|reg(0x%02x)",RegHi,RegLo);
  
  Stream << Form("%-18s%s\n",sreg.data(),text.data());
  
  TLOG(TLVL_DEBUG+1) << std::format("-- END");
}
  
//-----------------------------------------------------------------------------
// most of the time Link = -1 meaning 'all enabled links'
// otherwise it is the link to print
//-----------------------------------------------------------------------------
int DtcInterfaceCal::PrintRocStatus(uint32_t Format, int Link, std::ostream& Stream) {
  int rc(0);
    
  TLOG(TLVL_DBG) << Form("Format=%i Link:%i \n",Format,Link);
  
  std::string desc;
  
  int lnk1(Link), lnk2(Link+1);
  if (Link == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  uint reg;
  
  int link_mask(0);
  
  std::string text("        Register     ");
  for (int i=lnk1; i<lnk2; i++) {
    int enabled = LinkEnabled(i);
    if (enabled == 0)                                     continue;
    if (not LinkLocked(i)) {
      TLOG(TLVL_ERROR) << std::format("link:{} enabled but not locked",i);
      continue;
    }
    link_mask |= (1 << 4*i);
    text += Form("    ROC%i   ",i);
  }
  
  if (link_mask == 0) {
    std::string msg = std::format("dtc:{} link:{} : no locked links.",PcieAddr(),Link);
    Stream << " ERROR: " << msg << "\n";
    TLOG(TLVL_ERROR) << msg;
    return rc;
  }
                     
  if (Format != 0) text += " Description";
  Stream << Form("%s\n",text.data());
  Stream << "------------------------------------------------------------------------\n";
  
  // reg = ROC::Register::GTP_CRC; desc = "GTP_CRC";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream);
  
  // Stream << Form("\n");

  // uint reglo = ROC::Register::ActivePortsLow;
  // uint reghi = ROC::Register::ActivePortsHigh;
  // desc = "Active Ports";
  // PrintRocRegister2(reglo,reghi,desc,Format,link_mask,Stream); // hi goes first

  // reglo = ROC::Register::DRCnLow;
  // reghi = ROC::Register::DRCntHigh;
  // desc = "DR count";
  // PrintRocRegister2(reglo,reghi,desc,Format,link_mask,Stream);
  
  // Stream << Form("\n");

  // reg = ROC::Register::ID; desc = "ROC ID";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  // reg = ROC::Register::MarkerCnt; desc = "EWM count";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  // reg = ROC::Register::HeartBeat; desc = "HB count";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  // reg = ROC::Register::HeartBeatCn; desc = "HB count (fiber)";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  // reg = ROC::Register::MarkerDelay; desc = "marker delay";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  // reg = ROC::Register::HeartBeatCn; desc = "total N heartbeats";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 

  // reg = ROC::Register::Version; desc = "Version";
  // PrintRocRegister(reg,desc,Format,link_mask,Stream); // 
  
  Stream << "------------------------------------------------------------------------\n";
  TLOG(TLVL_DEBUG+1) << std::format("-- END");
  return rc;
}
