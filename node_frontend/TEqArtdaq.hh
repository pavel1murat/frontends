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

  int                 PrintProcesses(HNDLE H_Cmd);
  int                 ProcessStatus (HNDLE H_Cmd);
  int                 Tlvls         (HNDLE H_Cmd);
  int                 Treset        (HNDLE H_Cmd);
  int                 Tshow         (HNDLE H_Cmd);
  
  static void         ProcessCommand(int hDB, int hKey, void* Info);
  
  virtual int         StartMessage  (HNDLE h_Cmd, std::stringstream& Stream) override;
};
#endif
