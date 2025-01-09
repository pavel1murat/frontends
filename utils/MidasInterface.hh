#ifndef __MidasInterface_hh__
#define __MidasInterface_hh__

#include "midas.h"

class MidasInterface {

  static MidasInterface* _instance;

public:

  MidasInterface();
  
  static MidasInterface*  Instance  ();

  int get_environment        (char* HistName, int Len1, char* ExpName, int Len2);

  int connect_experiment     (const char *Host, const char *Experiment, const char* Client,
                              void (*func) (char *));
  
  int connect_experiment1    (const char *Host, const char *Experiment, const char* Client,
                              void (*func) (char *),
                              int odb_size, long int watchdog_timeout);

  int disconnect_experiment();
  
  int get_experiment_database(HNDLE * hDB, HNDLE * hKeyClient);

  int   db_enum_key(HNDLE hDB, HNDLE hDir , int Index, HNDLE* hComp);
  HNDLE db_find_key(HNDLE hDB, HNDLE hConf, const char* Key, HNDLE& Handle);
  int   db_get_key (HNDLE hDB, HNDLE hComp, KEY* Key); 
};

#endif
