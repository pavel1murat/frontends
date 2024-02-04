///////////////////////////////////////////////////////////////////////////////
// test of the Postgresql DB access and new run record creation
///////////////////////////////////////////////////////////////////////////////
#include "TRACE/tracemf.h"
#define  TRACE_NAME "db_runinfo_main"

#include "db_runinfo.hh"

int main(int argc, const char** argv) {

  db_runinfo x("aa");
//-----------------------------------------------------------------------------
// emulate new run
//-----------------------------------------------------------------------------
  if (argc != 1) {
    TLOG(TLVL_ERROR) << "N(par) != 1";
    return -1;
  }

  const char* runConfiguration = argv[0];
//-----------------------------------------------------------------------------
// step 1: request new run number, assume runConfiguration is described in ODB
// step 2: register 'start' and 'stop' trnsitions
//-----------------------------------------------------------------------------
  int rn = x.nextRunNumber(runConfiguration,0);

  x.registerTransition(rn,db_runinfo::START);
  x.registerTransition(rn,db_runinfo::STOP );
  return 0;
}
