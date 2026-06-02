//
#include <format>
#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

#include "TSystem.h"

#include "frontends/utils/OdbInterface.hh"
#include "frontends/utils/utils.hh"

//-----------------------------------------------------------------------------
int find_thresholds_panel(trkdaq::DtcInterface* Dtc_i, int Link, int Channel = -1, float VThreshold = 15, float VTolerance = 1, int PcieAddr = -1) {
  int rc(0);
  
  std::mutex mtx; // For thread-safe output

  uint16_t val[96][2];
  const char* type[2] = {"cal","hv"};

  int mnid = Dtc_i->ReadPanelID(Link);

  std::cout << std::format("-- START: making thresholds for mnid:{}\n",mnid);

  {
//-----------------------------------------------------------------------------
// start from timing alignment
//-----------------------------------------------------------------------------
    // std::lock_guard<std::mutex> lock(mtx);
    int n_bit_slips;
    rc = Dtc_i->FindAlignments(Link,n_bit_slips,0);
    if (rc < 0) {
      return rc;
    }
  }

  int ich1(Channel), ich2(Channel+1);
  if (Channel == -1) {
    ich1 = 0;
    ich2 = 96;
  }

  int nerrors;
  for (int ich=ich1; ich<ich2; ich++) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      std::cout << "-- channel:" << ich << std::endl;
    }

    for (int k=0; k<2; ++k) {
      nerrors = 0;
      bool ok(false);
      while ((not ok) and (nerrors < 3)) {
        {
          // std::lock_guard<std::mutex> lock(mtx);
          ok = Dtc_i->FindThreshold(Link,ich,k,VThreshold,VTolerance,val[ich][k]);
        }
        if (not ok) {
          //    std::lock_guard<std::mutex> lock(mtx);
          printf(" -- ERROR ich=%i k=%i nerrors:%i\n",ich,k,nerrors);
          nerrors += 1;
        }
        else {
          std::lock_guard<std::mutex> lock(mtx);
          printf("-- ich:%2i k:%i thr:%i\n",ich,k,val[ich][k]);
        }
      }
    }
  }
//-----------------------------------------------------------------------------
// try to figure the file name - if can get the MNID from ODB, use that,
// if not - use host-dtc_id-link -based name
//-----------------------------------------------------------------------------
  int pcie_addr = Dtc_i->PcieAddr();
  std::string fn, panel_name;

  fn = std::format("MN{:03}.json",mnid);
  
  {
    std::lock_guard<std::mutex> lock(mtx);
    std::ofstream of;
    of.open(fn);
 
    // after which one only needs to print the thresholds
    of << "[\n";
    for (int ich=ich1; ich<ich2; ich++) {
      for (int k=0; k<2; ++k) {
        of << std::format("{{\"channel\":{}",ich)
           << std::format(", \"type\":\"{}\"",type[k])
           << std::format(", \"threshold\":{}",val[ich][k])
           << std::format(", \"gain\":370}}");
        if ((k == 0) or (ich < ich2-1)) of << ",";
        of << std::endl;
      }
    }
    of << "]" << std::endl;
  }
  return 0;
}

// 2026-05-07 PM//-----------------------------------------------------------------------------
// 2026-05-07 PMint test_find_thresholds_mt(int Link, int Channel = -1, float VThreshold = 15, float VTolerance = 1, int PcieAddr = -1) {
// 2026-05-07 PM  std::vector<std::thread> threads;
// 2026-05-07 PM
// 2026-05-07 PM  int lnk1(Link), lnk2(Link+1);
// 2026-05-07 PM  if (Link == -1) {
// 2026-05-07 PM    lnk1 = 0;
// 2026-05-07 PM    lnk2 = 6;
// 2026-05-07 PM  }
// 2026-05-07 PM
// 2026-05-07 PM  for (int l=lnk1; l<lnk2; ++l) {
// 2026-05-07 PM    threads.emplace_back(find_thresholds_panel, Link, VThreshold, Channel, VTolerance, PcieAddr);
// 2026-05-07 PM  }
// 2026-05-07 PM
// 2026-05-07 PM  // Wait for all threads to complete
// 2026-05-07 PM  for (auto& t : threads) {
// 2026-05-07 PM    t.join();
// 2026-05-07 PM  }
// 2026-05-07 PM    
// 2026-05-07 PM  std::cout << "All threads completed!" << std::endl;
// 2026-05-07 PM  return 0;
// 2026-05-07 PM}


//-----------------------------------------------------------------------------
// NodeLabel: short node name , i.e. "mu2e-trk-18"
//-----------------------------------------------------------------------------
int find_thresholds(const char* NodeLabel, int PcieAddr, int Link1, int Link2, float VThreshold = 15, int Channel = -1, float VTolerance = 1) {
  std::vector<std::thread> threads;

  auto dtc_i = dtc_init(NodeLabel,PcieAddr);
  dtc_i->InitReadout(0,1);
  
  for (int lnk=Link1; lnk<Link2+1; lnk++) {
    find_thresholds_panel(dtc_i,lnk, Channel, VThreshold, VTolerance, PcieAddr);
  }

  std::cout << __func__ << " completed!" << std::endl;
  return 0;
}
