//-----------------------------------------------------------------------------
#define __CLING__ 1

#include "iostream"
#include "nlohmann/json.hpp"

#include "midas.h"
#include "odbxx.h"
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
