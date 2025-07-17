///////////////////////////////////////////////////////////////////////////////

#include "node_frontend/TEquipmentNode.hh"
#include "utils/utils.hh"

#include "midas.h"
#include "odbxx.h"

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"

//-----------------------------------------------------------------------------
void TEquipmentNode::InitTrkDtcVarNames(int PcieAddr) {

  // SC TODO: I think we should define these together with the registers? 
  std::initializer_list<const char*> dtc_names = {"Temp", "VCCINT", "VCCAUX", "VCBRAM"};

  const std::string eq_path     = "/Equipment/"+TMFeEquipment::fEqName;
  //midas::odb odb_node(eq_path);
  
  const std::string settings_path = eq_path+"/Settings";
  midas::odb odb_settings(node_path+"/Settings");
  
  std::vector<std::string> dtc_var_names;
  for (const char* name : dtc_names) {
    sprintf(var_name,"dtc%i#%s",PcieAddr,name);
    dtc_var_names.push_back(name);
  }
       
  sprintf(dirname,"Names dtc%i",PcieAddr);
  odb_settings[dirname] = dtc_var_names;
//-----------------------------------------------------------------------------
// non-history DTC registers
//-----------------------------------------------------------------------------
  dtc_var_names.clear();
  for (const int& reg : DtcRegisters) {
    // sprintf(var_name,"dtc%i#0x%04x",PcieAddr,reg);
    sprintf(var_name,"0x%04x",reg);
    dtc_var_names.push_back(var_name);
  }
      
  sprintf(dirname,"DTC%i",PcieAddr);

  midas::odb   odb_dtc(node_path+"/"+dirname);
  odb_dtc["RegName"] = dtc_var_names;

  std::vector<uint32_t> dtc_reg_data(dtc_var_names.size());
  odb_dtc["RegData"] = dtc_reg_data;
//-----------------------------------------------------------------------------
// loop over the ROCs and create names for each of them
//-----------------------------------------------------------------------------
  for (int ilink=0; ilink<6; ilink++) { 
     
    std::vector<std::string> roc_var_names;
    for (int k=0; k<trkdaq::TrkSpiDataNWords; k++) {
      sprintf(var_name,"rc%i%i#%s",idtc,ilink,trkdaq::DtcInterface::SpiVarName(k));
      roc_var_names.push_back(var_name);
    }
      
    sprintf(dirname,"Names rc%i%i",idtc,ilink);
    if (not midas::odb::exists(settings_path+"/"+dirname)) {
      odb_settings[dirname] = roc_var_names;
    }
//-----------------------------------------------------------------------------
// non-history ROC registers - counters and such - just to be looked at
//-----------------------------------------------------------------------------
    roc_var_names.clear();
    for(const int& reg : RocRegisters) {
      sprintf(var_name,"reg_%03i",reg);
      roc_var_names.push_back(var_name);
    }

    char roc_subdir[128];

    sprintf(roc_subdir,"%s/DTC%i/ROC%i",node_path.data(),idtc,ilink);
    midas::odb odb_roc = {{"RegName",{"a","b"}},{"RegData",{1u,2u}}};
    odb_roc.connect(roc_subdir);
    odb_roc["RegName"] = roc_var_names;
    
    std::vector<uint16_t> roc_reg_data(roc_var_names.size());
    odb_roc["RegData"] = roc_reg_data;
  }
  
  return;
}
