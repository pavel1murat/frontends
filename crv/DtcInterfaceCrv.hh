//-----------------------------------------------------------------------------
// interactive interface for ROOT-based GUI
// mixes high- and low-level commands
// assume everything is happening on one node
// there could be one or two DTCs and only one CFO
// TODO : interface change :
// 1. std::ostream& --> std::ostream* or use std::ostream(nullptr)
//    to avoid ifs in the functions being called
//-----------------------------------------------------------------------------
#ifndef __frontends_crv_dtcinterfacecrv_hh__
#define __frontends_crv_drcinterfacecrv_hh__

#define __CLING__ 1

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "dtcInterfaceLib/DTC.h"
#include "artdaq-core-mu2e/Overlays/DTC_Types/DTC_Link_ID.h"
#include "artdaq-core-mu2e/Overlays/DTC_Packets/DTC_RocDataHeaderPacket.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterfaceBase.hh"

class DtcInterfaceCrv : public mu2edaq::DtcInterface { 
private:
  DtcInterfaceCrv(int PcieAddr, uint LinkMask, bool SkipInit);
public:

  DtcInterfaceCrv(DTCLib::DTC* Dtc);

  // struct RocData_t {                    // 8 16-bit words in total
  //   RocDataHeaderPacket_t header;
  //   uint16_t              data[1];
  // };
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
public:
  static  DtcInterfaceCrv* Instance  (int PcieAddr, uint LinkMask = 0x11, bool SkipInit = false);

  virtual int           InitRocReadoutMode(std::ostream& Stream = std::cout)      override;
//-----------------------------------------------------------------------------
// Format =  0 : for each register, print a register and its value
// Format =  1 : add short description of each register
// Link   = -1 : print a line per register for each ROC
//-----------------------------------------------------------------------------
  void         PrintRocRegister  (uint32_t Reg, std::string& Desc, int Format = 1, int LinkMask = -1, std::ostream& Stream = std::cout);
  void         PrintRocRegister2 (uint32_t RegLo, uint32_t RegHi, std::string& Desc, int Format = 1, int LinkMask = -1, std::ostream& Stream = std::cout);
  virtual int  PrintRocStatus    (uint32_t Format = 1, int Link = -1, std::ostream& Stream = std::cout) override;
  void         PrintSpiAll       (trkdaq::TrkSpiData_t* Spi, std::ostream& Stream = std::cout);
    
  virtual int  ResetLink         (int Link) override;

  int          RocConfigure      (int Link, bool  gr, uint16_t grn, uint16_t uBoffset, uint16_t timeout, std::ostream& Stream = std::cout);
  int          SetMarkerSync     (int Link, bool enable);
//-----------------------------------------------------------------------------
// return number of found errors
//-----------------------------------------------------------------------------
  virtual int  Validate          (ushort* Data, ulong EwTag, ulong* Offset, int PrintLevel, int* NErrRoc) override ;
};

#endif
