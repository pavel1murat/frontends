///////////////////////////////////////////////////////////////////////////////
// test_json_003: test reading a thresholds file
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <format>
#include <fstream>
#include "nlohmann/json.hpp"
//-----------------------------------------------------------------------------
// works interactively
//-----------------------------------------------------------------------------
namespace ns {
  struct parameters {
    std::string eq_type;
    int         pcie;
    int         roc;
  };
  
  void to_json(nlohmann::json& j, const parameters& p) {
    j = nlohmann::json{ {"eq_type", p.eq_type}, {"pcie", p.pcie},{"roc", p.roc} };
  }
  
  void from_json(const nlohmann::json& j, parameters& p) {
    try {
      j.at("eq_type").get_to(p.eq_type);
      j.at("pcie").get_to(p.pcie);
      j.at("roc").get_to(p.roc);
    }
    catch (const nlohmann::json::exception& e) {
      std::cerr << "JSON parsing error: " << e.what() << std::endl;
      throw;
    }
  }
}

//-----------------------------------------------------------------------------
int test_json_002(const char* String = "{\"eq_type\":\"dtc\",\"pcie\":0,\"roc\":0}") {

  nlohmann::json j1 = nlohmann::json::parse(String);
  std::cout << "eq_type:" << j1.at("eq_type") << " pcie:" << j1.at("pcie") << " roc:" << j1.at("roc") << std::endl;
  return 0;
}

//-----------------------------------------------------------------------------
// read a thresholds file and plot thresholds
//-----------------------------------------------------------------------------
int test_json_003(const char* Filename) {
  
  std::ifstream ifs(Filename);
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
  // fill histograms

  TH1F* h_cal = new TH1F("thr_cal","thr_cal",500,250,750);
  h_cal->SetLineColor(kRed+2);
  
  TH1F* h_hv  = new TH1F("thr_hv" ,"thr_hv" ,500,250,750);
  
  for (int i=0; i<96; i++) {
    h_cal->Fill(thr_cal[i]);
    h_hv->Fill (thr_hv [i]);
  }

  h_cal->Draw();
  h_hv->Draw("sames");
  return 0;
}

//-----------------------------------------------------------------------------
// read a thresholds file and plot thresholds
//-----------------------------------------------------------------------------
int compare_thresholds(const char* Fn1, const char* Fn2) {
  
  int thr_cal_1[96], thr_hv_1[96], thr_cal_2[96], thr_hv_2[96];

  std::ifstream ifs(Fn1);
  nlohmann::json jf = nlohmann::json::parse(ifs);

  for (auto& elm : jf.items()) {
    nlohmann::json o = elm.value();
    int ich  = o["channel"];
    int gain = o["gain"];
    int thr  = o["threshold"];
    std::string type = o["type"];
    std::cout << std::format("{:2d} {:3d} {:3d} {}\n",ich,gain,thr,type);
    if (type == "cal") thr_cal_1[ich] = thr;
    else               thr_hv_1 [ich] = thr;
  }

  std::ifstream ifs_2(Fn2);
  nlohmann::json jf_2 = nlohmann::json::parse(ifs_2);

  for (auto& elm : jf_2.items()) {
    nlohmann::json o = elm.value();
    int ich  = o["channel"];
    int gain = o["gain"];
    int thr  = o["threshold"];
    std::string type = o["type"];
    std::cout << std::format("{:2d} {:3d} {:3d} {}\n",ich,gain,thr,type);
    if (type == "cal") thr_cal_2[ich] = thr;
    else               thr_hv_2 [ich] = thr;
  }
  
  // fill histograms

  TH1F* h_cal_1 = new TH1F("thr_cal_1","thr_cal_1",500,250,750);
  
  TH1F* h_hv_1  = new TH1F("thr_hv_1" ,"thr_hv_1" ,500,250,750);
  
  for (int i=0; i<96; i++) {
    h_cal_1->Fill(thr_cal_1[i]);
    h_hv_1->Fill (thr_hv_1 [i]);
  }

  TH1F* h_cal_2 = new TH1F("thr_cal_2","thr_cal_2",500,250,750);
  h_cal_2->SetLineColor(kRed+2);
  
  TH1F* h_hv_2  = new TH1F("thr_hv_2" ,"thr_hv_2" ,500,250,750);
  h_hv_2->SetLineColor(kRed+2);
  
  for (int i=0; i<96; i++) {
    h_cal_2->Fill(thr_cal_2[i]);
    h_hv_2->Fill (thr_hv_2 [i]);
  }

  TCanvas* c = new TCanvas("c","c",1600,800);
  c->Divide(2,2);
  
  c->cd(1);
  h_cal_1->Draw();
  h_cal_2->Draw("sames");

  c->cd(2);
  h_hv_1->Draw();
  h_hv_2->Draw("sames");

  TH1F* h_dcal = new TH1F("d_thr_cal","d_thr_cal",100,-50,50);
  TH1F* h_dhv  = new TH1F("dthr_hv" ,"dthr_hv" ,100,-50,50);

  for (int i=0; i<96; i++) {
    h_dcal->Fill(thr_cal_2[i]-thr_cal_1[i]);
    h_dhv->Fill (thr_hv_2 [i]-thr_hv_1 [i]);
  }
  
  c->cd(3);
  h_dcal->Draw();

  c->cd(4);
  h_dhv->Draw();

  return 0;
}


//-----------------------------------------------------------------------------
// test reading a program_drac config file
//-----------------------------------------------------------------------------
int test_json_004(const char* Filename) {
  
  std::ifstream ifs(Filename);
  nlohmann::json jf = nlohmann::json::parse(ifs);

  for (auto& elm : jf.items()) {
    nlohmann::json o = elm.value();
    std::string version  = o["version"];
    int fpga_image_present  = o["fpga_image"]["present"];
    std::cout << std::format("version:{:10} fpga_image_present:{}\n",version,fpga_image_present);
  }
  
  return 0;
}


//-----------------------------------------------------------------------------
int test_json(int Test, const char* Filename = "settings_vadim.json") {
  int rc(0);
  
  if (Test == 3) {
    rc = test_json_003(Filename);
  }
  return rc;
}
