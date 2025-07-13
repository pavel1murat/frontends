///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
//-----------------------------------------------------------------------------
// works interactively
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
    std::cout << ich << " " << gain << " " << std::setw(3) << thr << " " << type << std::endl;;
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
