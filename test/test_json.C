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
// test reading a thresholds file
//-----------------------------------------------------------------------------
int test_json_003(const char* Filename) {
  
  std::ifstream ifs(Filename);
  nlohmann::json jf = nlohmann::json::parse(ifs);

  for (auto& elm : jf.items()) {
    nlohmann::json o = elm.value();
    int ich  = o["channel"];
    int gain = o["gain"];
    int thr  = o["threshold"];
    std::string type = o["type"];
    std::cout << std::format("{:2d} {:3d} {:3d} {}\n",ich,gain,thr,type);
  }
  
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
