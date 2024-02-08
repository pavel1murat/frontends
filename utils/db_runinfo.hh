#ifndef __db_runinfo_hh__
#define __db_runinfo_hh__

#include <string>
#include <libpq-fe.h> /* for PGconn */

class db_runinfo {
public: 
  enum TransitionType  {
    HALT   = 0,                         // RUNNING --> HALTED  (abort   )
    STOP   = 1,                         // RUNNING --> STOPPED (stop    )
    ERROR  = 2,                         // ERROR               (not sure)
    PAUSE  = 3,                         // RUNNING --> PAUSED  (pause   )
    RESUME = 4,                         // PAUSED  --> RUNNING (resume  )
    START  = 5                          // STOPPED --> RUNNING (start   )
  };

  std::string  _uid;
  int          _debugLevel;

  std::string  _dbname;
  std::string  _dbhost;
  std::string  _dbport;
  std::string  _dbuser;
  std::string  _dbpwd ;

	std::string  _dbSchema;

	PGconn*      _runInfoDbConn;

  db_runinfo (const char* UID, int DebugLevel = 0);
  ~db_runinfo();

//-----------------------------------------------------------------------------
// run configuration should contain all needed information. By default, store 
// the new run number in ODB
// TransitionCause: 0:start 1:end
//-----------------------------------------------------------------------------
  int nextRunNumber     (const char* RunConfiguration, int StoreInODB=1);
  int registerTransition(int RunNumber, uint TransitionType, uint CauseType);

	int  openConnection ();
	int  checkConnection();
};

#endif
