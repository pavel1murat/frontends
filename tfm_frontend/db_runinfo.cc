///////////////////////////////////////////////////////////////////////////////
// P.M. 
// - the ODB connection should be established by the caller, 
// - the same is true for disconnecting from ODB
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define  TRACE_NAME "db_runinfo"

#include "midas.h"

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
    TLOG(TLVL_ERROR) << " Postgresql DB user is not defined";
    throw("db_runinfo::db_runinfo: Postgresql DB user is not defined"); 
  }

  int rc = openConnection();
  TLOG(TLVL_DEBUG) << "after openConnection rc=" << rc;
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

  TLOG(TLVL_DEBUG) << "connecting to Postgresql";

  if (PQstatus(_runInfoDbConn) == CONNECTION_OK)            return 0;

  TLOG(TLVL_ERROR) << "Unable to connect to run_info database!";
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
    TLOG(TLVL_ERROR) << "INSERT INTO 'run_transition' DATABASE TABLE FAILED!!! PQ ERROR: " 
                     << PQresultErrorMessage(res);
    rc = -1;
  }

  PQclear(res);

  return rc;
}

//-----------------------------------------------------------------------------
// integrate with ODB
//-----------------------------------------------------------------------------
int db_runinfo::nextRunNumber(const char* RunConfiguration, int StoreInODB) { // int RunType, const std::string& RunInfoConditions) {
  int   runNumber(-1);
  int   runType  (-1);
  int   partition_number(-1); 

  char  trigger_table_name[100]; // trigger table version is a "_vXX" stub in the end, ok to have just one string
  int   rc(0);
//-----------------------------------------------------------------------------
  rc = checkConnection();
  if (rc < 0) return rc;
//-----------------------------------------------------------------------------
// retrieve from ODB parameters to be stored in Postgresql
//-----------------------------------------------------------------------------
  HNDLE       hDB, hKey;
  rc = cm_get_experiment_database(&hDB, NULL);
  if (rc != CM_SUCCESS) {
    TLOG(TLVL_ERROR) << "failed to connect to ODB";
    return rc;
  }

  char run_conf_key[200];
  sprintf(run_conf_key,"/Experiment/RunConfigurations/%s",RunConfiguration);
	db_find_key(hDB, 0, run_conf_key, &hKey);

  int sz = sizeof(runType);
  db_get_value(hDB, hKey, "RunType", &runType, &sz, TID_INT, FALSE);

  sz = sizeof(partition_number);
  db_get_value(hDB, hKey, "ARTDAQ_PARTITION_NUMBER", &partition_number, &sz, TID_INT, FALSE);

  sz = sizeof(trigger_table_name);
  db_get_value(hDB, hKey, "TriggerTable", trigger_table_name, &sz, TID_STRING, FALSE);

  const char* hostName        = getenv("HOSTNAME");
//-----------------------------------------------------------------------------
// write run info into db
//-----------------------------------------------------------------------------
  PGresult* res;
  char      buffer[1024];

  std::string runConfigurationVersion = "1";
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
                      VALUES ('%d','%s','%d','%s','%s','%s','%s',CURRENT_TIMESTAMP);",
           _dbSchema,
           runType,
           hostName,
           partition_number,
           RunConfiguration,
           runConfigurationVersion.c_str(),
           trigger_table_name,"1");

  res = PQexec(_runInfoDbConn, buffer);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    PQclear(res);
    return -1;
  }
  PQclear(res);

  snprintf(buffer,sizeof(buffer),"select max(run_number) from %s.run_configuration;",_dbSchema);

  res = PQexec(_runInfoDbConn, buffer);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    TLOG(TLVL_ERROR) << "RUN INFO SELECT FROM PG run_configuration TABLE FAILED!!! PQ message:" 
                     << PQresultErrorMessage(res);
    PQclear(res);
    return -2;
  }

  if (PQntuples(res) == 1) {
    runNumber = atoi(PQgetvalue(res, 0, 0));
  }
  else {
    TLOG(TLVL_ERROR) << "RUN NUMBER retrieval FROM the DB FAILED!!! PQ ERROR:" << PQresultErrorMessage(res);
    PQclear(res);
    return -3;
  }
  PQclear(res);
//-----------------------------------------------------------------------------
// P.M. hopefully, the next part will not be needed
// insert a new row in the run_condition table if 
// it pk(runConfiguration,runConfigurationVersion, runContext,runContextVersion) doesn't exist yet
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// these are going away
//-----------------------------------------------------------------------------
  std::string runContext        = "no_context";
  std::string runContextVersion = "0";

  snprintf(buffer,
           sizeof(buffer),
           "SELECT configuration_name, configuration_version, context_name, context_version \
            FROM   %s.run_condition WHERE \
            configuration_name  = '%s'  AND \
            configuration_version = '%s'  AND \
            context_name      = '%s'  AND \
            context_version   = '%s';",
           _dbSchema,
           RunConfiguration,
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

    std::string runInfoConditions = "unknown";
    std::string pkRun = RunConfiguration + runConfigurationVersion + runContext + runContextVersion;
    if (strcmp(pk.c_str(), pkRun.c_str())) {
      PQclear(res);
      char buffer2[4192];
      snprintf(buffer2,sizeof(buffer2),
               "INSERT INTO %s.run_condition(configuration_name, configuration_version, context_name, \
                context_version, condition, commit_time)  VALUES ('%s','%s','%s','%s','%s',CURRENT_TIMESTAMP);",
               _dbSchema,
               RunConfiguration,
               runConfigurationVersion.c_str(),
               runContext.c_str(),
               runContextVersion.c_str(),
               runInfoConditions.c_str());
        
      res = PQexec(_runInfoDbConn, buffer2);
        
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cout << "INSERT INTO 'run_condition' DATABASE TABLE FAILED!!! PQ ERROR: " << PQresultErrorMessage(res)
                  << std::endl;
        PQclear(res);
        return -6;
      }
    }
  }
    
  PQclear(res);
  
  if (StoreInODB) {
//-----------------------------------------------------------------------------
// instead of printing, store the next run number directly in ODB
// MIDAS increments the next run number, so subtract one in advance
//-----------------------------------------------------------------------------
    int rn = runNumber-1;
    rc = db_set_value(hDB, 0, "/Runinfo/Run number",&rn, sizeof(rn), 1, TID_INT32);
    if (rc != CM_SUCCESS) {
      TLOG(TLVL_ERROR) << "couldnt store the new run number=" << runNumber << "in ODB";
      return -7;
    }
  }

  if (runNumber == -1) {
    TLOG(TLVL_ERROR) << "couldnt get new run number from Postgresql";
    return -8;
  }

  return runNumber;
}
 
