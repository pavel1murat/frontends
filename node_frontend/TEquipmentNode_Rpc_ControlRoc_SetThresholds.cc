/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"
//
#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
// #include "nlohmann/json.hpp"
#include "odbxx.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void TEquipmentNode::SetThresholds_Thread(void* Context) {
  //int PcieAddr, int Link, trkdaq::DtcInterface* Dtc_i,
  //                                               std::ostream& Stream, const char* ConfName) {

  ThreadContext_t* context = (ThreadContext_t*) Context;
  
  // int pcie_addr = context->fPcieAddr;

  midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_ROC_set_thresholds");

  int doit = o["Doit"] ;
  
  int lnk1 = context->fLink;
  int lnk2 = lnk1+1;
  if (context->fLink == -1) {
    lnk1 = 0;
    lnk2 = 6;
  }
  
  for (int lnk=lnk1; lnk<lnk2; lnk++) {
//-----------------------------------------------------------------------------
// this could be universal,
// with the definition of DetectorElement being subsystem-dependent
//-----------------------------------------------------------------------------
    std::string  panel_path = std::format("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{:s}/DTC{:d}/Link{:d}/DetectorElement",
                                          context->fEqNode->_host_label.data(),
                                          context->fPcieAddr,lnk);
    midas::odb o(panel_path);

    (*context->Stream) << " -- link:" << lnk << std::endl;
    (*context->Stream) << "ich mask G_cal  G_hv Th_cal Th_hv" << std::endl;
    (*context->Stream) << "---------------------------------" << std::endl;

    for (int i=0; i<96; i++) {

      int ch_mask  = o["ch_mask"][i];
      int gain_cal = o["gain_cal"][i];
      int thr_cal  = o["threshold_cal"][i];
      int gain_hv  = o["gain_hv"][i];
      int thr_hv   = o["threshold_hv"][i];
      
      (*context->Stream) << std::format("{:3d} {:3d}  {:3d} {:3d} {:3d} {:3d}\n",
                                        i,ch_mask,gain_cal,gain_hv,thr_cal,thr_hv);
      
      if (doit != 0) {
        // TThread::Lock();
        trkdaq::DtcInterface* dtc_i = (trkdaq::DtcInterface*) context->fDtc_i;
        try {
          dtc_i->ControlRoc_SetThreshold(lnk,i,0,thr_cal );
          dtc_i->ControlRoc_SetGain     (lnk,i,0,gain_cal);
        
          dtc_i->ControlRoc_SetThreshold(lnk,i,1,thr_hv );
          dtc_i->ControlRoc_SetGain     (lnk,i,1,gain_hv);
        }
        catch(...) {
          (*context->Stream) << "ERROR : coudn't execute Rpc_ControlRoc_SetThresholds. BAIL OUT" << std::endl;
          break;
        }
        // TThread::Lock();
      }
    }
  }
}
