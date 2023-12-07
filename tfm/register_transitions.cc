///////////////////////////////////////////////////////////////////////////////
// interface to MIDAS: get next run number and print it on the screen
// perhaps migrate to python ? - shoudl be a few-liner
///////////////////////////////////////////////////////////////////////////////
#include "db_runinfo.hh"

int main(int argc, const char** argv) {

  db_runinfo x("aa");
//-----------------------------------------------------------------------------
// emulate new run
//-----------------------------------------------------------------------------
  
//   int rn = x.nextRunNumber();
  int rn = atoi(argv[1]);

  if (rn > 0) printf("%i\n",rn);
//-----------------------------------------------------------------------------
// transitions are registered by the clients
// a test - see db_runinfo_main.cc
//-----------------------------------------------------------------------------
  x.registerTransition(rn,db_runinfo::START);
  x.registerTransition(rn,db_runinfo::STOP );
  return 0;
}
