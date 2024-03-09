///////////////////////////////////////////////////////////////////////////////
// test of the Postgresql DB access and new run record creation
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define  TRACE_NAME "db_runinfo_main"

#include "midas.h"
#include "db_runinfo.hh"

#define DEFAULT_TIMEOUT  10000       /* 10 seconds for watchdog timeout */

int main(int argc, const char** argv) {
//-----------------------------------------------------------------------------
// emulate new run
//-----------------------------------------------------------------------------
  if (argc < 2) {
    TLOG(TLVL_ERROR) << "N(par) < 1 : " << argc-1;
    return -1;
  }

  const char* runConfiguration = argv[1];
//-----------------------------------------------------------------------------
// step 1: request new run number, assume runConfiguration is described in ODB
// step 2: register 'start' and 'stop' trnsitions
//-----------------------------------------------------------------------------
  std::string hostname, expname;
  cm_get_environment(&hostname, &expname);

  int status = cm_connect_experiment1(hostname.data(), expname.data(), "db_runinfo",
                                      NULL, DEFAULT_ODB_SIZE, DEFAULT_TIMEOUT);
  if (status != CM_SUCCESS) return -2;

  db_runinfo x("aa");

  int rn = x.nextRunNumber(runConfiguration,0);

  TLOG(TLVL_INFO) << "run_conf:" << runConfiguration << " new run number : " << rn ;

  x.registerTransition(rn,db_runinfo::START,0);
  ss_sleep(1);
  x.registerTransition(rn,db_runinfo::START,1);
  x.registerTransition(rn,db_runinfo::STOP ,0);
  ss_sleep(1);
  x.registerTransition(rn,db_runinfo::STOP ,1);

  cm_disconnect_experiment();

  return 0;
}
