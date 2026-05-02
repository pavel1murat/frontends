//-----------------------------------------------------------------------------
// for a given station, read thresholds from .json files and plot them 
//-----------------------------------------------------------------------------
#include <format>
#include "daqana/obj/TrkPanelMap_t.hh"

int plot_thresholds(int Slot, int Threshold_MV) {

  // const char* Fn1, const char* Fn2) {
  
  std::string dir = std::format("config/tracker/slot_{:02}/thresholds-{}-mV",Slot,Threshold_MV);

  TrkPanelMap_t* tpm = TrkPanelMap_t::Instance(121103);

  TH1F* h_cal[2][6];
  TH1F* h_hv [2][6];

  for (int plane=2*Slot; plane<2*Slot+2; plane++) {
    for (int panel=0; panel<6; panel++) {
      
      TrkPanelMap_t::Data_t* tpmd = tpm->panel_data_by_offline(plane,panel);
      int mnid = tpmd->mnid;
      std::string fn = std::format("{}/MN{:03d}.json",dir,mnid);
      std::cout << std::format(" .. opening {}\n",fn);
      std::ifstream ifs(fn);
      nlohmann::json jf = nlohmann::json::parse(ifs);

      int thr_cal[96], thr_hv[96];
      
      for (auto& elm : jf.items()) {
        nlohmann::json o = elm.value();
        int ich  = o["channel"];
        int gain = o["gain"];
        int thr  = o["threshold"];
        std::string type = o["type"];
        std::cout << std::format("{:2d} {:3d} {:3d} {}\n",ich,gain,thr,type);
        if (type == "cal") thr_cal[ich] = thr;
        else               thr_hv [ich] = thr;
      }

      int ip = plane % 2;
      
      std::string hname;
      hname            = std::format("h_cal_MN{:03}_{}_{}",mnid,plane,panel);
      h_cal[ip][panel] = new TH1F(hname.data(),hname.data(),500,250,750);
      hname            = std::format("h_hv_MN{:03}_{}_{}",mnid,plane,panel);
      h_hv [ip][panel] = new TH1F(hname.data(),hname.data(),500,250,750);
      h_hv[ip][panel]->SetLineColor(kRed+2);

      for (int i=0; i<96; i++) {
        h_cal[ip][panel]->Fill(thr_cal[i]);
        h_hv [ip][panel]->Fill(thr_hv [i]);
      }
    }
  }
//-----------------------------------------------------------------------------
// now plot the histograms
//-----------------------------------------------------------------------------
  std::string cname;
  cname      = std::format("c_slot_{:02}",Slot);
  TCanvas* c = new TCanvas(cname.data(),cname.data(),1600,900);
  c->Divide(4,3);

  int ip = 0;
  for (int dtc=0; dtc<2; dtc++) {
    for (int lnk=0; lnk<6; lnk++) {
      c->cd(ip+1);
      h_cal[dtc][lnk]->Draw();
      h_hv [dtc][lnk]->Draw("sames");
      ip++;
    }
  }

  return 0;
}
