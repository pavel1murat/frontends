///////////////////////////////////////////////////////////////////////////////
// for debugging in interactive mode
///////////////////////////////////////////////////////////////////////////////
#include "db_runinfo.hh"

int main(int argc, const char** argv) {

  db_runinfo x("aa");
//-----------------------------------------------------------------------------
// emulate new run
//-----------------------------------------------------------------------------
  int rn = x.nextRunNumber();

  if (rn > 0) printf("%i",rn);

  // x.registerTransition(rn,db_runinfo::START);
  // x.registerTransition(rn,db_runinfo::STOP );
  return 0;
}
