//-----------------------------------------------------------------------------
#define __CLING__ 1

#include "iostream"
#include "string"
#include "nlohmann/json.hpp"

#include "midas.h"
#include "odbxx.h"
#include "frontends/utils/OdbInterface.hh"
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int print_trk_panel_map(int Station, const char* Experiment = "tracker") {

  cm_connect_experiment("mu2e-dl-01-data", Experiment, "print_trk_panel_map",NULL);

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h_tracker = odb_i->GetHandle(0,"/Mu2e/ActiveRunConfiguration/Tracker");

  HNDLE h_station = odb_i->GetHandle(h_tracker,std::format("Station_{:02d}",Station));

  for (int plane=0; plane<2; plane++) {
    HNDLE h_plane = odb_i->GetHandle(h_station,std::format("Plane_{:02d}",plane));
    HNDLE h_dtc = odb_i->GetHandle(h_plane,"DTC");
    int dtc_id = odb_i->GetDtcID(h_dtc);
    std::string plane_name = odb_i->GetString(h_plane,"Name");
    // std::cout << std::format("# plane_name:{}\n",plane_name);
    int ppid = std::stoi(plane_name.substr(6));
    for (int panel=0; panel<6; panel++) {
      HNDLE h_panel = odb_i->GetHandle(h_station,std::format("Plane_{:02d}/Panel_{:02d}",plane,panel));
      std::string panel_name = odb_i->GetString(h_panel,"Name");
      int mnid = std::stoi(panel_name.substr(2));
      int link = odb_i->GetInteger(h_panel,"Link");
      int pln  = odb_i->GetInteger(h_panel,"GeoPlane") % 2;
      int pnl  = odb_i->GetInteger(h_panel,"GeoPanel");
      int zfc  = pln*2+(odb_i->GetInteger(h_panel,"slot_id") % 10)/3;

      // std::cout << std::format("name:{} mnid:{:03d} dtc_id:{:02} link:{} ppid:{:2} pln:{} pnl:{} zfc:{}\n",
      //                          panel_name,mnid,dtc_id,link,ppid,pln,pnl,zfc);

      std::cout << std::format("   {:03},  {:3}, {:3}, {:3}, {:4}, {:2}, {:3}",
                               mnid,dtc_id,link,pln,ppid,pnl,zfc) << std::endl;
    }
  }

  // std::cout << "data_dir:" << data_dir << std::endl;
  //  
  cm_disconnect_experiment();
  return 0;
}
