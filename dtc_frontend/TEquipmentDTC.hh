//-----------------------------------------------------------------------------
// DTC 
//-----------------------------------------------------------------------------
#ifndef __TEquipmentDTC_hh__
#define __TEquipmentDTC_hh__

#include "tmfe.h"
#include "midas.h"

#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

class TEquipmentDTC : public TMFeEquipment {
  enum {
    kNRegHist    = 4,
    kNRegNonHist = 15+2*6,     
  } ;
  
  const int RegHist[kNRegHist] = {
    0x9010, 0x9014, 0x9018, 0x901c      // Temp, VCCINT, VCCAUX, VCBRAM
  };

 const int RegNonHist[kNRegNonHist] = {
   0x9004, 0x9100, 0x9114, 0x9140, 0x9144,
   0x9158, 0x9188, 0x91a8, 0x91ac, 0x91bc, 
   0x91c0, 0x91c4, 0x91f4, 0x91f8, 0x93e0,

   0x9630, 0x9650,                      // link 0: N(DTC requests) and N(heartbeats) 
   0x9634, 0x9654,                      // Link 1: N(DTC requests) and N(heartbeats) 
   0x9638, 0x9658,                      // Link 2: N(DTC requests) and N(heartbeats) 
   0x963c, 0x965c,                      // Link 3: N(DTC requests) and N(heartbeats) 
   0x9640, 0x9660,                      // Link 4: N(DTC requests) and N(heartbeats) 
   0x9644, 0x9664                       // Link 5: N(DTC requests) and N(heartbeats) 
 };
 
public:

  HNDLE  hDB;

  trkdaq::DtcInterface* fDtc_i[2];       // one or two DTCs, nullprt:disabled
  
  TEquipmentDTC(const char* eqname, const char* eqfilename);

  virtual TMFeResult HandleInit(const std::vector<std::string>& args);
  virtual void       HandlePeriodic();

};
#endif
