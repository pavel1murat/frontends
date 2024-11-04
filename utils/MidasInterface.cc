//

#include "midas.h"
#include "utils/MidasInterface.hh"

MidasInterface* MidasInterface::_instance(nullptr);

//-----------------------------------------------------------------------------
MidasInterface::MidasInterface() {
}

//-----------------------------------------------------------------------------
MidasInterface* MidasInterface::Instance() {
  if (_instance == nullptr) {
    _instance = new MidasInterface();
  }
  return _instance;
}

//-----------------------------------------------------------------------------
int MidasInterface::get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient) {
  return cm_get_experiment_database(hDB,hKeyClient);
}

//-----------------------------------------------------------------------------
int MidasInterface::get_environment(char* HostName, int Len1, char* ExpName, int Len2) {
  return cm_get_environment(HostName,Len1,ExpName,Len2);
}

//-----------------------------------------------------------------------------
int MidasInterface::connect_experiment(const char *Host, const char *Experiment,
                                       const char* Client,
                                       void (*Func) (char *)) {

  return cm_connect_experiment(Host,Experiment,Client,Func);
}

//-----------------------------------------------------------------------------
int MidasInterface::connect_experiment1(const char *Host, const char *Experiment,
                                       const char* Client,
                                        void (*Func) (char *),
                                        int OdbSize, long int WatchdogTimeout) {

  return cm_connect_experiment1(Host,Experiment,Client,Func,OdbSize,WatchdogTimeout);
}

//-----------------------------------------------------------------------------
int MidasInterface::disconnect_experiment() {
  cm_disconnect_experiment();
}

