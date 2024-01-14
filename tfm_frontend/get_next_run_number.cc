///////////////////////////////////////////////////////////////////////////////
// interface to MIDAS: get next run number from Postgresql and store it in ODB
// perhaps migrate to python ? - should be a few-liner
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define  TRACE_NAME "get_next_run_number"

#include "midas.h"
#include "db_runinfo.hh"

#define DEFAULT_TIMEOUT  10000       /* 10 seconds for watchdog timeout */


int main(int argc, const char** argv) {
  HNDLE       hDB(0); // , hKey;
  int         rc(0);

  if (argc < 1) {
    TLOG(TLVL_ERROR) << "wrong number of command line parameters";
    return -1;
  }

  char host_name[100], exp_name[100];
  cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));

  int status = cm_connect_experiment1(host_name, exp_name, "get_next_run_number",
                                      NULL, DEFAULT_ODB_SIZE, DEFAULT_TIMEOUT);
  if (status != CM_SUCCESS) {
    cm_msg(MERROR, "mainFE", "Cannot connect to experiment \'%s\' on host \'%s\', status %d", exp_name, host_name,
           status);
    /* let user read message before window might close */
    ss_sleep(5000);
    return 1;
  }

  rc = cm_get_experiment_database(&hDB, NULL);
  if (rc != CM_SUCCESS) {
    TLOG(TLVL_ERROR) << "couldnt connect to  ODB";
    return -2;                  // no need to disconnect
  }
//-----------------------------------------------------------------------------
// open new connection to Postgresql
//-----------------------------------------------------------------------------
  db_runinfo x("aa");

  const char* run_configuration = argv[1];

  int rn = x.nextRunNumber(run_configuration,1);
  if (rn < 0) {
//-----------------------------------------------------------------------------
// if rn < 0, it is an error return code. Print an error and exit
//-----------------------------------------------------------------------------
    rc = rn;
    TLOG(TLVL_ERROR) << "couldnt define new run number, rc=" << rc;
  }

  cm_disconnect_experiment();
  return rc;
}
