//-----------------------------------------------------------------------------
#define __CLING__ 1

#include "iostream"
#include "nlohmann/json.hpp"

#include "midas.h"
#include "odbxx.h"
#include "frontends/utils/OdbInterface.hh"

//-----------------------------------------------------------------------------
int test_odb_access() {

  cm_connect_experiment(NULL, NULL, "test_025", NULL);

  HNDLE hdb;
  cm_get_experiment_database(&hdb, NULL);

  HNDLE h_dtc;
  db_find_key(hdb,0,"/Mu2e/ActiveRunConfiguration/Tracker/PanelMap/400/MN400/DTC",&h_dtc);

  HNDLE h2;
  char name[128];

  db_find_key(hdb, h_dtc, name,&h2);
  printf("name: %s\n",name);

  HNDLE h_parent;
  db_get_parent(hdb,h2,&h_parent);

  KEY key;
  db_get_key(hdb,h_parent,&key);
  printf("key.name: %s\n",key.name);
  
  cm_disconnect_experiment();
  return 0;
}

//-----------------------------------------------------------------------------
int test_odb_link() {

  cm_connect_experiment(NULL, NULL, "test_025", NULL);

  HNDLE hdb;
  cm_get_experiment_database(&hdb, NULL);

  HNDLE h_dtc;
  db_find_key(hdb,0,"/Mu2e/ActiveRunConfiguration/Tracker/PanelMap/400/MN400/DTC",&h_dtc);

  HNDLE h2;
  char name[128];

  db_find_key(hdb, h_dtc, name,&h2);
  printf("name: %s\n",name);

  HNDLE h_parent;
  db_get_parent(hdb,h2,&h_parent);

  KEY key;
  db_get_key(hdb,h_parent,&key);
  printf("key.name: %s\n",key.name);
  
  cm_disconnect_experiment();
  return 0;
}

//-----------------------------------------------------------------------------
int test_odb_array() {

  cm_connect_experiment(NULL, NULL, "test_025", NULL);

  OdbInterface* odb_i = OdbInterface::Instance();

  int t1 = odb_i->GetUInt16(0,"/Mu2e/RunConfigurations/roctower/Tracker/Station_99/Plane_00/Panel_00/thr_hv[0]");
  int t2 = odb_i->GetUInt16(0,"/Mu2e/RunConfigurations/roctower/Tracker/Station_99/Plane_00/Panel_00/thr_hv[1]");
                    
  std::cout << "t1:" << t1 << " t2:" << t2 << std::endl;
  
  cm_disconnect_experiment();
  return 0;
}

//-----------------------------------------------------------------------------
// so far, looks pretty much useless
//-----------------------------------------------------------------------------
int test_odb_path() {

  cm_connect_experiment(NULL, NULL, "test_025", NULL);

  OdbInterface* odb_i = OdbInterface::Instance();

  HNDLE h = odb_i->GetHandle(0,"/Mu2e/ActiveConfiguration");
  HNDLE h1 = odb_i->GetHandle(h,"Tracker");

  std::string path = db_get_path(odb_i->GetDbHandle(),h1);

  std::cout << "path:" << path << std::endl;
  
  cm_disconnect_experiment();
  return 0;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int test_get_string(const char* Experiment="tracker") {

  cm_connect_experiment(NULL, Experiment, "test_get_string",NULL);

  OdbInterface* odb_i = OdbInterface::Instance();

  std::string data_dir = odb_i->GetString(0,"/Logger/Data dir");

  std::cout << "data_dir:" << data_dir << std::endl;
  
  cm_disconnect_experiment();
  return 0;
}
