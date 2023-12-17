///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "db_runinfo.hh"
#include <boost/algorithm/string.hpp>

//==============================================================================
db_runinfo::db_runinfo(const char* UID, int DebugLevel) {
  _uid      = UID;

  _dbname   = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE"       ) ? getenv("OTSDAQ_RUNINFO_DATABASE"       ) : "run_info");
  _dbhost   = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE_HOST"  ) ? getenv("OTSDAQ_RUNINFO_DATABASE_HOST"  ) : "");
  _dbport   = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE_PORT"  ) ? getenv("OTSDAQ_RUNINFO_DATABASE_PORT"  ) : "");
  _dbuser   = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE_USER"  ) ? getenv("OTSDAQ_RUNINFO_DATABASE_USER"  ) : "");
  _dbpwd    = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE_PWD"   ) ? getenv("OTSDAQ_RUNINFO_DATABASE_PWD"   ) : "");
  _dbSchema = (const char*) (getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA") ? getenv("OTSDAQ_RUNINFO_DATABASE_SCHEMA") : "test");

  _debugLevel  = DebugLevel;

  if (_dbuser[0] == 0) {
    printf("ERROR in db_runinfo::db_runinfo : _dbuser is not defined\n");
    throw("db_runinfo::db_runinfo: Postgres DB user is not defined"); 
  }

  int rc = openConnection();
  if (DebugLevel > 0) printf("db_runinfo::db_runinfo constructor: connection: %i\n",rc);
}

//==============================================================================
db_runinfo::~db_runinfo() {
  PQfinish(_runInfoDbConn);
}

//==============================================================================
int db_runinfo::openConnection() {
  char runInfoDbConnInfo [1024];

  sprintf(runInfoDbConnInfo, "dbname=%s host=%s port=%s user=%s password=%s", 
          _dbname, _dbhost, _dbport, _dbuser, _dbpwd);

  _runInfoDbConn = PQconnectdb(runInfoDbConnInfo);

  if (_debugLevel > 0) printf("db_runinfo::%s connecting...\n",__func__);

  if (PQstatus(_runInfoDbConn) == CONNECTION_OK)            return 0;

  printf("ERROR in db_runinfo::%s : Unable to connect to run_info database!\n",__func__);
  PQfinish(_runInfoDbConn);
  return -999;
}

//==============================================================================
int db_runinfo::checkConnection() {
  if (PQstatus(_runInfoDbConn) == CONNECTION_OK) return 0;
//-----------------------------------------------------------------------------
// if connection is bad, try one more time to re-open it
//-----------------------------------------------------------------------------
  PQfinish(_runInfoDbConn);
  return openConnection();
}

//==============================================================================
int db_runinfo::registerTransition(int RunNumber, uint TransitionType) {

  int       rc(0);
  PGresult* res;
  char      buffer[1024];

  rc = checkConnection();
  if (rc < 0) return rc;
//-----------------------------------------------------------------------------
// connection OK, register transition
//-----------------------------------------------------------------------------
  snprintf(buffer,sizeof(buffer),
           "INSERT INTO %s.run_transition(run_number, \
                                          transition_type, \
                                          transition_time)    \
                      VALUES (%ld,'%d',CURRENT_TIMESTAMP);",
           _dbSchema, (long int) (RunNumber), TransitionType);

  res = PQexec(_runInfoDbConn,buffer);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::cout << "INSERT INTO 'run_transition' DATABASE TABLE FAILED!!! PQ ERROR: " 
              << PQresultErrorMessage(res)
              << std::endl;
    rc = -1;
  }

  PQclear(res);

  return rc;
}

//==============================================================================
int db_runinfo::nextRunNumber(const std::string& RunInfoConditions) {
  int runNumber = -1;

  // const char* mu2eOwner       = getenv("MU2E_OWNER");
  const char* hostName        = getenv("HOSTNAME");
  const char* artadqPartition = getenv("ARTDAQ_PARTITION");

  int rc(0);

  rc = checkConnection();
  if (rc < 0) return rc;
//-----------------------------------------------------------------------------
// write run info into db
//-----------------------------------------------------------------------------
  PGresult* res;
  char      buffer[1024];

  // extract configuration name and version from runInfoConditions
  std::string runConfiguration = "vst_001";

  std::string runConfigurationVersion = "1";

  // extract context name and version from runInfoConditions
  std::string runContext = "no_context";

  std::string runContextVersion = "0";
//-----------------------------------------------------------------------------
// P.M. one should not pass the runtype via environment, but... 
//-----------------------------------------------------------------------------
  const char* rt      = getenv("MU2E_RUNTYPE");
  const char* runType =  ( rt ? rt : "1");
//-----------------------------------------------------------------------------
//  at this point run number in the DB gets incremented
//-----------------------------------------------------------------------------
  snprintf(buffer,sizeof(buffer),
           "INSERT INTO %s.run_configuration( \
                        run_type \
                      , host_name \
                      , artdaq_partition \
                      , configuration_name \
                      , configuration_version \
                      , trigger_table_name \
                      , trigger_table_version \
                      , commit_time) \
                      VALUES ('%s','%s','%d','%s','%s','%s','%s',CURRENT_TIMESTAMP);",
           _dbSchema,
           runType,
           hostName,
           std::stoi(artadqPartition),
           runConfiguration.c_str(),
           runConfigurationVersion.c_str(),
           "tracker_vst_trigger_table","1");

  res = PQexec(_runInfoDbConn, buffer);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    PQclear(res);
    return -1;
  }
  PQclear(res);

  snprintf(buffer,sizeof(buffer),"select max(run_number) from %s.run_configuration;",_dbSchema);

  res = PQexec(_runInfoDbConn, buffer);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    printf("RUN INFO SELECT FROM DATABASE TABLE FAILED!!! PQ ERROR: %s",PQresultErrorMessage(res));
    PQclear(res);
    return -2;
  }

  if (PQntuples(res) == 1) {
    runNumber = atoi(PQgetvalue(res, 0, 0));
  }
  else {
    printf("RUN NUMBER retrieval FROM the DB FAILED!!! PQ ERROR: %s",PQresultErrorMessage(res));
    PQclear(res);
    return -3;
  }
  PQclear(res);
//-----------------------------------------------------------------------------
// P.M. hopefully, the next part will not be needed
// insert a new row in the run_condition table if 
// it pk(runConfiguration,runConfigurationVersion, runContext,runContextVersion) doesn't exist yet
//-----------------------------------------------------------------------------
  snprintf(buffer,
           sizeof(buffer),
           "SELECT configuration_name, configuration_version, context_name, context_version \
            FROM   %s.run_condition WHERE \
            configuration_name  = '%s'  AND \
            configuration_version = '%s'  AND \
            context_name      = '%s'  AND \
            context_version   = '%s';",
           _dbSchema,
           runConfiguration.c_str(),
           runConfigurationVersion.c_str(),
           runContext.c_str(),
           runContextVersion.c_str());
  
  res = PQexec(_runInfoDbConn, buffer);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    std::cout << "SELECT FROM 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res) << std::endl;
    PQclear(res);
    return -5;
  }
//-----------------------------------------------------------------------------
// get primary key in the run_condition table
//-----------------------------------------------------------------------------
  if (PQntuples(res) != 1) {
    std::string pk;
    int nFields = PQnfields(res);
    for (int i = 0; i < PQntuples(res); i++) {
      for (int j = 0; j < nFields; j++) {
        pk += PQgetvalue(res, i, j);
      }
    }

    std::cout << pk << std::endl;

    std::string pkRun = runConfiguration + runConfigurationVersion + runContext + runContextVersion;
    if (strcmp(pk.c_str(), pkRun.c_str())) {
      PQclear(res);
      char buffer2[4192];
      snprintf(buffer2,sizeof(buffer2),
               "INSERT INTO %s.run_condition(configuration_name, configuration_version, context_name, \
                context_version, condition, commit_time)  VALUES ('%s','%s','%s','%s','%s',CURRENT_TIMESTAMP);",
               _dbSchema,
               runConfiguration.c_str(),
               runConfigurationVersion.c_str(),
               runContext.c_str(),
               runContextVersion.c_str(),
               RunInfoConditions.c_str());
        
      res = PQexec(_runInfoDbConn, buffer2);
        
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cout << "INSERT INTO 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
                  << std::endl;
        PQclear(res);
        return -6;;
      }
    }
  }
    
  PQclear(res);

  if (runNumber == -1) {
    std::cout << "Impossible run number not defined by run info plugin!" << std::endl;
    return -7;
  }

  return runNumber;
}
 
