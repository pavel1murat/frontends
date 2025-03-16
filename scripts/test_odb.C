
//

#include "midas.h"
#include "utils/MidasInterface.hh"

// //-----------------------------------------------------------------------------
// int test_odb_001() {

//   MidasInterface* mi = MidasInterface::Instance();

//   mi->connect_experiment("mu2edaq22-ctrl","test_025","test_odb_001",nullptr);

//   HNDLE hDB, hClient;
//   mi->get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient);
  
//   HNDLE h_conf = mi->get_;
//   KEY   component;
//   int   ncomp(0);
//   for (int i=0; db_enum_key(hDB, h_artdaq_conf, i, &h_component) != DB_NO_MORE_SUBKEYS; ++i) {
//     db_get_key(hDB, h_component, &component);
//     printf("Subkey %d: %s, Type: %d\n", i, component.name, component.type);
//     ncomp++;
//   }

//   return 0;
// }

//-----------------------------------------------------------------------------
int test_odb_002() {

  MidasInterface* mi = MidasInterface::Instance();

  mi->connect_experiment("mu2edaq22-ctrl","test_025","test_odb_002",nullptr);

  HNDLE hDB, hClient;
  mi->get_experiment_database(&hDB, &hClient);

  printf("%s: hDB = %i\n",__func__, hDB);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);
  
  HNDLE h;
  int rc = mi->db_find_key(hDB,0,"/Mu2e/ActiveRunConfiguration",h);

  
  std::string name = odb_i->GetString(hDB,h,"Name");
  printf("name = %s\n",name.data());
  
  return rc;
}

//-----------------------------------------------------------------------------
int test_odb_003() {

  MidasInterface* mi = MidasInterface::Instance();

  mi->connect_experiment("mu2edaq22-ctrl","test_025","test_odb_002",nullptr);

  HNDLE hDB, hClient;
  mi->get_experiment_database(&hDB, &hClient);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);

  HNDLE h_conf = odb_i->GetActiveRunConfigHandle();
  
  std::string name = odb_i->GetRunConfigName(h_conf);

  printf("name = %s\n",name.data());

  return 0;
}

//-----------------------------------------------------------------------------
// test setting the ROC ID for DTC1:Link2
//-----------------------------------------------------------------------------
int test_odb_004(int DtcID = 1, int Link = 2) {

  if (Link == -1) {
    printf("ERROR\n");
    return -1;
  }

  MidasInterface* mi = MidasInterface::Instance();

  mi->connect_experiment("mu2edaq22-ctrl","test_025","test_odb_002",nullptr);

  HNDLE hDB, hClient;
  mi->get_experiment_database(&hDB, &hClient);

  OdbInterface* odb_i = OdbInterface::Instance(hDB);

  HNDLE h_conf = odb_i->GetActiveRunConfigHandle();
  
  std::string name = odb_i->GetRunConfigName(h_conf);

  printf("name = %s\n",name.data());


  trkdaq::DtcInterface* dtc_i = trkdaq::DtcInterface::Instance(DtcID);
  std::string roc_id      = dtc_i->GetRocID         (Link);
  std::string design_info = dtc_i->GetRocDesignInfo (Link);
  std::string git_commit  = dtc_i->GetRocFwGitCommit(Link);

  std::string p0 = Form("/Mu2e/ActiveRunConfiguration/DAQ/Nodes/mu2edaq22/DTC%i/Link%i",DtcID,Link);
  printf("p0 = %s\n",p0.data());

  HNDLE h_link = odb_i->GetHandle(hDB,0,p0.data());

  odb_i->SetRocID         (h_link,roc_id);
  odb_i->SetRocDesignInfo (h_link,design_info);
  odb_i->SetRocFwGitCommit(h_link,git_commit);

  //  mi->disconnect_experiment();

  return 0;
}
