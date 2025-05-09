//-----------------------------------------------------------------------------
// iterate over the ODB subdirectory  and count the number of subkeys in it
//-----------------------------------------------------------------------------
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include "midas.h"

int main(int argc, char **argv) {
  HNDLE hDB, hKey;
  INT   status, num_subkeys;
  KEY   key;

  cm_connect_experiment     (NULL, NULL, "Example", NULL);
  cm_get_experiment_database(&hDB, NULL);

  char dir[] = "/Mu2e/RunConfigurations/train_station/DAQ/Nodes/mu2edaq09";

  status = db_find_key(hDB, 0, dir , &hKey);
  if (status != DB_SUCCESS) {
    printf("Error: Cannot find the ODB directory\n");
    return 1;
  }
//-----------------------------------------------------------------------------
// Iterate over all subkeys in the directory
// note: key.num_values is NOT the number of subkeys in the directory
//-----------------------------------------------------------------------------
  db_get_key(hDB, hKey, &key);
  printf("key.num_values: %d\n",key.num_values);

  HNDLE hSubkey;
  KEY   subkey;
  num_subkeys = 0;
  for (int i=0; db_enum_key(hDB, hKey, i, &hSubkey) != DB_NO_MORE_SUBKEYS; ++i) {
    db_get_key(hDB, hSubkey, &subkey);
    printf("Subkey %d: %s, Type: %d\n", i, subkey.name, subkey.type);
    num_subkeys++;
  }

  printf("number of subkeys: %d\n",num_subkeys);

  std::string link_path = "/Mu2e/RunConfigurations/train_station/DAQ/Nodes/mu2edaq09/DTC1/Link0";
  HNDLE h_link;
  status = db_find_key(hDB, 0, link_path.data() , &h_link);
  std::cout  << "link_path:" <<  link_path << " h_link:" << h_link << std::endl;

  HNDLE h_panel;
  db_find_key(hDB, h_link, "DetectorElement", &h_panel);
  
  std::cout << "h_panel:" <<  h_panel << std::endl;


  char name[100];
  int nel = 100;

  HNDLE h_name;
  
  db_find_key(hDB,h_panel,"Name",&h_name);

  db_get_data(hDB,h_name,name,&nel,TID_STRING);
  
  std::cout << "name:" <<  name << std::endl;
  // Disconnect from MIDAS
  cm_disconnect_experiment();
  return 0;
}
