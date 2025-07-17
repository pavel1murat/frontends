/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
#include "otsdaq-mu2e-tracker/Ui/DtcInterface.hh"

#include "node_frontend/TEqTrkDtc.hh"
#include "utils/OdbInterface.hh"
#include "utils/utils.hh"
#include "nlohmann/json.hpp"
#include "odbxx.h"

//-----------------------------------------------------------------------------
int TEqTrkDtc::ReadSpi(std::ostream& Stream) {
  int rc(0);
  //   midas::odb o   ("/Mu2e/Commands/Tracker/DTC/control_roc_read_spi");

  HNDLE h_cmd                = odb_i->GetDtcCommandHandle();
  std::string parameter_path = odb_i->GetString(h_cmd,"ParameterPath");
  HNDLE h_cmd_par            = odb_i->GetHandle(h_cmd,parameter_path);

  int link         = odb_i->GetInteger(h_cmd_par,"link"       ); // o["link"       ];
  int print_level  = odb_i->GetInteger(h_cmd_par,"print_level"); // o["print_level"];
  
  try         {
    if (link != -1) {
      std::vector<uint16_t>   data;
      dtc_i->ControlRoc_ReadSpi(data,link,print_level,ss);
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
  catch (...) {
    Stream << "ERROR : coudn't read SPI ... BAIL OUT" << std::endl;
    rc = -1;
  }

  return rc;
}
