//-----------------------------------------------------------------------------
// ARTDAQ 
//-----------------------------------------------------------------------------
#ifndef __TEqArtdaq_hh__
#define __TEqArtdaq_hh__

#include <ctime>
#include "midas.h"

#include "utils/TMu2eEqBase.hh"
#include "node_frontend/ArtdaqComponent.hh"

class TEqArtdaq: public TMu2eEqBase {
public:
  int                            _cmd_run;
  std::vector<ArtdaqComponent_t> _list_of_ac;
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
  TEqArtdaq(const char* Name, const char* Title);
  ~TEqArtdaq();

  int                 ReadBrMetrics(const ArtdaqComponent_t* Ac);
  int                 ReadDrMetrics(const ArtdaqComponent_t* Ac);
  int                 ReadDsMetrics(const ArtdaqComponent_t* Ac);

  virtual TMFeResult  Init          () override;
  virtual int         InitVarNames  () override;
  virtual int         HandlePeriodic() override;

  int                 ReadMetrics   ();

  int                 PrintProcesses(std::ostream& Stream);
  int                 ProcessStatus (std::ostream& Stream);
  int                 Tlvls         (std::ostream& Stream);
  int                 Tshow         (std::ostream& Stream, const std::string& Logfile);
  
  static void         ProcessCommand(int hDB, int hKey, void* Info);
  
};
#endif
