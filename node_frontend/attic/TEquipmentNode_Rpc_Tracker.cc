/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/CfoInterface.hh"
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEquipmentNode.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "odbxx.h"

using namespace std;

#include "TRACE/tracemf.h"
#define  TRACE_NAME "TEquipmentNode"


//-----------------------------------------------------------------------------
// this function needs to define which type of equipment is being talked to and forward
//-----------------------------------------------------------------------------
TMFeResult TEquipmentNode::Rpc_Tracker(mu2edaq::DtcInterface* Dtc_i, HNDLE hDtc, const char* cmd, const char* args, std::ostream& Stream) {
  fMfe->Msg(MINFO, "Rpc_Tracker", "RPC cmd [%s], args [%s]", cmd, args);

  OdbInterface* odb_i      = OdbInterface::Instance();
  HNDLE         h_run_conf = odb_i->GetActiveRunConfigHandle();
  std::string   conf_name  = odb_i->GetRunConfigName(h_run_conf);

  std::string conf_path("/Mu2e/RunConfigurations/");
  conf_path += conf_name;

  TLOG(TLVL_DEBUG) << "-- START: RPC cmd:" << cmd << " args:" << args << " conf_name:" << conf_name;
//-----------------------------------------------------------------------------
// in principle, string could contain a list of parameters to be parsed
// so far : only PCIE address
// so parse parametrers : expect a string in JSON format, like '{"pcie":1}' 
//-----------------------------------------------------------------------------
  trkdaq::DtcInterface* dtc_i;
  try {
    dtc_i = dynamic_cast<trkdaq::DtcInterface*>(Dtc_i);
  }
  catch(...) {
    TLOG(TLVL_ERROR) << "failed to initialize the DTC";
    Stream << " failed to initialize DTC" << Dtc_i->PcieAddr();
  }

  if (dtc_i == nullptr) {
    Stream << " DTC" << pcie_addr << " is not enabled";
    TLOG(TLVL_ERROR) << "failed to initialize DTC" << Dtc_i->PcieAddr();
    return TMFeErrorMessage(response);
  }
//-----------------------------------------------------------------------------
// at this point we know that we're dealing with the tracker DTC
//-----------------------------------------------------------------------------
  if (strcmp(cmd,"configure_ja") == 0) {
//-----------------------------------------------------------------------------
// configure the jitter attenuator. Use dtc_i->fJAMode
//-----------------------------------------------------------------------------
    int rc = dtc_i->ConfigureJA();
    if (rc == 0) Stream << " SUCCESS";
    else         Stream << " ERROR: failed to configure the JA";
  }
  else if (strcmp(cmd,"dtc_control_roc_read") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_read, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    Stream << std::endl;
    Rpc_ControlRoc_Read(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"dtc_control_roc_read_ddr") == 0) {
    Stream << std::endl;
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_read_ddr");
    Rpc_ControlRoc_ReadDDR(dtc_i,roc,ss);
  }
  else if (strcmp(cmd,"dtc_control_roc_digi_rw") == 0) {
//-----------------------------------------------------------------------------
// for control_ROC_digi_rw, it would make sense to have a separate page
//-----------------------------------------------------------------------------
    Stream << std::endl;
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_digi_rw");

    trkdaq::ControlRoc_DigiRW_Input_t  par;
    trkdaq::ControlRoc_DigiRW_Output_t pout;
    // parameters should be taken from ODB - where from?

    par.rw           = o["rw"     ];    //
    par.hvcal        = o["hvcal"  ];    //
    par.address      = o["address"];    //
    par.data[0]      = o["data"   ][0]; //
    par.data[1]      = o["data"   ][1]; //
    int  print_level = o["print_level"];
   
    printf("dtc_i->fLinkMask: 0x%04x\n",dtc_i->fLinkMask);
    try {
      dtc_i->ControlRoc_DigiRW(&par,&pout,roc,print_level,ss);
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_DigiRW ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_read_register") == 0) {
    int      timeout_ms(150);

    midas::odb o("/Mu2e/Commands/Tracker/DTC/read_register");
 
    try {
      uint32_t reg = o["Register"];
      uint32_t val;
      dtc_i->fDtc->GetDevice()->read_register(reg,timeout_ms,&val);
      o["Value"] = val;
      Stream << " -- read_dtc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { Stream << " ERROR : dtc_read_register ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_write_register") == 0) {
    try {
      int      timeout_ms(150);
      midas::odb o("/Mu2e/Commands/Tracker/DTC/write_register");
      uint32_t reg = o["Register"];
      uint32_t val = o["Value"];
      Stream << " -- write_dtc_register::0x" << std::hex << reg << " val:0x" << val << std::dec;
      dtc_i->fDtc->GetDevice()->write_register(reg,timeout_ms,val);
    }
    catch (...) { Stream << "ERROR : DTC write register failed. BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_soft_reset") == 0) {
    try         { dtc_i->Dtc()->SoftReset(); Stream << "soft reset OK" << std::endl; }
    catch (...) { Stream << "ERROR : coudn't soft reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_hard_reset") == 0) {
    try         { dtc_i->Dtc()->HardReset(); Stream << "hard reset OK" << std::endl; }
    catch (...) { Stream << "ERROR : coudn't hard reset the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_init_readout") == 0) {
    try         {
      midas::odb o("/Mu2e/Commands/Tracker/DTC/init_readout");
      uint32_t emulate_cfo      = o["emulate_cfo"     ];
      uint32_t roc_readout_mode = o["roc_readout_mode"];
      dtc_i->InitReadout(emulate_cfo,roc_readout_mode);
      Stream << "DTC:" << pcie_addr << " emulate_cfo:" << emulate_cfo
         << " roc_readout_mode:" << roc_readout_mode << " init readout OK";
    }
    catch (...) { Stream << "ERROR : coudn't init readout DTC:" << pcie_addr; }
  }
  else if (strcmp(cmd,"dtc_print_status") == 0) {
    Stream << std::endl;
    try         { dtc_i->PrintStatus(ss); }
    catch (...) { Stream << "ERROR : coudn't print status of the DTC ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_update_registers") == 0) {
    try         {
      ReadNonHistDtcRegisters(dtc_i);
    }
    catch (...) { Stream << "ERROR : coudn't update DTC non-hist registers... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_print_roc_status") == 0) {
    Stream << std::endl;
    try         { dtc_i->PrintRocStatus(1,-1,ss); }
    catch (...) { Stream << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"dtc_control_roc_find_alignment") == 0) {
    if (roc == -1)  Stream << std::endl;
    try         { dtc_i->FindAlignments(1,roc,ss); }
    catch (...) { Stream << " -- ERROR : coudn't execute FindAlignments for link:" << roc << " ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"get_key") == 0) {
//-----------------------------------------------------------------------------
// get key data
//-----------------------------------------------------------------------------
    Stream << std::endl;
    try         {
      std::vector<uint16_t> data;
      dtc_i->ControlRoc_GetKey(data,roc,2,ss);
    }
    catch (...) { Stream << "ERROR : coudn't execute ControlRoc_GetKey ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"get_roc_design_info") == 0) {
//-----------------------------------------------------------------------------
// get ROC design info - print output of 3 separate commands together
//-----------------------------------------------------------------------------
    Stream << std::endl;
    int rmin(roc), rmax(roc+1);
    if (roc == -1) {
      rmin = 0;
      rmax = 6;
    }
   
    TLOG(TLVL_INFO) << "-------------- rmin, rmax:" << rmin << " " << rmax;

    for (int i=rmin; i<rmax; ++i) {
      Stream << "----- link:" << i ;
      if (dtc_i->LinkEnabled(i) == 0) {
        Stream << " disabled, continue" << std::endl;
      }
      // ROC ID
      try         {
        std::string roc_id = dtc_i->GetRocID(i);
        Stream << std::endl;
        Stream << "roc_id         :" << roc_id << std::endl;
      }
      catch (...) {
        Stream << "ERROR : coudn't execute GetRocID ... BAIL OUT" << std::endl;
      }
      // design info 
      try         {
        std::string design_info = dtc_i->GetRocDesignInfo(i);
        Stream << "roc_design_info:" << design_info << std::endl;
      }
      catch (...) {
        Stream << "ERROR : coudn't execute GetRocDesignInfo ... BAIL OUT" << std::endl;
      }
      // git commit
      try {
        std::string git_commit = dtc_i->GetRocFwGitCommit(i);
        Stream << "git_commit     :" << "'"  << git_commit << "'" << std::endl;
      }
      catch (...) {
        Stream << "ERROR : coudn't execute GetRocFwGitCommit ... BAIL OUT" << std::endl;
      }
    }
  }
  else if (strcmp(cmd,"measure_thresholds") == 0) {
//-----------------------------------------------------------------------------
// measure thresholds
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at measure_thresholds";
    Stream << std::endl;
    ThreadContext_t cxt(pcie_addr,roc,2);
    MeasureThresholds(cxt,ss);
  }
  else if (strcmp(cmd,"load_thresholds") == 0) {
//-----------------------------------------------------------------------------
// load thresholds
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at load_thresholds";
    if (roc == -1) Stream << std::endl;
    ThreadContext_t cxt(pcie_addr,roc);
    LoadThresholds(cxt,ss);             // this is fast, synchronous
  }
  else if (strcmp(cmd,"set_thresholds") == 0) {
//-----------------------------------------------------------------------------
// set thresholds: use thread as the RPC times out
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at set_thresholds";
    // Stream << std::endl;
    fSetThrContext.fPcieAddr = pcie_addr;
    fSetThrContext.fLink     = roc;

    // SetThresholds(fSetThrContext,*this,ss);
    std::thread set_thr_thread(SetThresholds,
                               std::ref(fSetThrContext),
                               std::ref(*this),
                               std::ref(ss)
                               );
    set_thr_thread.detach();
    // add sleep to be able to print the output
    ss_sleep(5000);
  }
  else if (strcmp(cmd,"read_roc_register") == 0) {
//-----------------------------------------------------------------------------
// ROC registers are 16-bit
//-----------------------------------------------------------------------------
    try         {
      int timeout_ms(150);

      midas::odb o("/Mu2e/Commands/Tracker/DTC/read_roc_register");
      uint16_t reg = o["Register"];
      uint16_t val = dtc_i->Dtc()->ReadROCRegister(DTC_Link_ID(roc),reg,timeout_ms);
      o["Value"]   = val;
      Stream << " -- read_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { Stream << " -- ERROR : coudn't read ROC register ... BAIL OUT"; }
  }
  else if (strcmp(cmd,"dtc_control_roc_pulser_on") == 0) {
//-----------------------------------------------------------------------------
// turn pulser ON - lik is passed explicitly
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_on";
    std::string parameter_path("/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on");

    int first_channel_mask, duty_cycle, pulser_delay, print_level;
    try {
      midas::odb o(parameter_path);

      first_channel_mask = o["first_channel_mask"];    //
      duty_cycle         = o["duty_cycle"        ];    //
      pulser_delay       = o["pulser_delay"      ];    //
      print_level        = o["print_level"       ];
    }
    catch(...) {
      Stream << " -- ERROR : coudn't read parameters from " << parameter_path << " ... BAIL OUT" << std::endl;
    }

    TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOn, roc:" << roc;
    try {
      dtc_i->ControlRoc_PulserOn(roc,first_channel_mask,duty_cycle,pulser_delay,print_level,ss);
    }
    catch(...) {
      Stream << " -- ERROR : coudn\'t execute ControlRoc_PulserOn ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_control_roc_pulser_off") == 0) {
//-----------------------------------------------------------------------------
// turn pulser OFF
//-----------------------------------------------------------------------------
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_pulser_off";
    midas::odb o("/Mu2e/Commands/Tracker/DTC/control_ROC_pulser_off");

    int print_level        = o["print_level"];

    TLOG(TLVL_DEBUG) << "trying to call ControlRoc_PulserOff, roc:" << roc;
    try {
      dtc_i->ControlRoc_PulserOff(roc,print_level,ss);
    }
    catch(...) {
      Stream << "ERROR : coudn't execute ControlRoc_PulserOFF ... BAIL OUT" << std::endl;
    }
  }
  else if (strcmp(cmd,"dtc_control_roc_rates") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    Stream << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dtc_control_roc_rates";
 
     Rpc_ControlRoc_Rates(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"set_caldac") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    Stream << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at set_caldaq";
 
     Rpc_ControlRoc_SetCalDac(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"dump_settings") == 0) {
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
    Stream << std::endl;
    TLOG(TLVL_DEBUG) << "arrived at dump_settings";
 
    Rpc_ControlRoc_DumpSettings(pcie_addr,roc,dtc_i,ss,conf_name.data());
  }
  else if (strcmp(cmd,"read_ilp") == 0) {
//-----------------------------------------------------------------------------
// read ILP
//-----------------------------------------------------------------------------
    Stream << std::endl;
    try         {
      std::vector<uint16_t>   data;
      dtc_i->ControlRoc_ReadIlp(data,roc,2,ss);
    }
    catch (...) { Stream << "ERROR : coudn't read ILP ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"read_spi") == 0) {
//-----------------------------------------------------------------------------
// get formatted SPI output for a given ROC
//-----------------------------------------------------------------------------
    Stream << std::endl;
    try         {
      if (roc != -1) {
        std::vector<uint16_t>   data;
        dtc_i->ControlRoc_ReadSpi(data,roc,2,ss);
      }
      else {
        // need formatted printout for all ROCs
        trkdaq::TrkSpiData_t spi[6];
        for (int i=0; i<6; i++) {
          if (dtc_i->LinkEnabled(i)) {
            dtc_i->ControlRoc_ReadSpi_1(&spi[i],i,0,ss);
          }
        }
        // now - printing
        dtc_i->PrintSpiAll(spi,ss);
      }
    }
    catch (...) { Stream << "ERROR : coudn't read SPI ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"write_roc_register") == 0) {
    try         {
      int timeout_ms(150);

      midas::odb o("/Mu2e/Commands/Tracker/DTC/write_roc_register");
      uint16_t reg = o["Register"];
      uint16_t val = o["Value"];
      dtc_i->Dtc()->WriteROCRegister(DTC_Link_ID(roc),reg,val,false,timeout_ms);
      Stream << " -- write_roc_register:0x" << std::hex << reg << " val:0x" << val << std::dec;
    }
    catch (...) { Stream << "ERROR : coudn't write ROC register ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"print_roc_status") == 0) {
    Stream << std::endl;
    try         {
      dtc_i->PrintRocStatus(1,1<<4*roc,ss);
      Stream << " -- print_roc_status : emoe";
    }
    catch (...) { Stream << "ERROR : coudn't print ROC status ... BAIL OUT" << std::endl; }
  }
  else if (strcmp(cmd,"reset_roc") == 0) {
    try         {
      int mask = 1<<4*roc;
      dtc_i->ResetLinks(mask);
      Stream << " -- reset_roc OK";
    }
    catch (...) { Stream << "ERROR : coudn't reset ROC:" << roc << " ... BAIL OUT" << std::endl; }
  }
  else {
    Stream << " ERROR: Unknown command:" << cmd;
    TLOG(TLVL_ERROR) << ss.str();
  }

  response += ss.str();

  TLOG(TLVL_DEBUG) << "-- END: response:" << response;

  ss.str("");

  return TMFeOk();
}
