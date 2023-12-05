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

  const char* dbname_;
  const char* dbhost_;
  const char* dbport_;
  const char* dbuser_;
  const char* dbpwd_ ;

	const char* dbSchema_;

	PGconn*     runInfoDbConn_;

  db_runinfo (const char* UID);
  ~db_runinfo();

  int nextRunNumber(const std::string& RunInfoConditions = "");

  int registerTransition(int RunNumber, int TransitionType);
  int updateRunInfo     (int RunNumber, int TransitionType);

	void openConnection ();
	int  checkConnection();
};

#endif
