// 2024-12-27 P.Murat: compared to a year ago, there are changes in the output format, parsing will have to change
#ifndef __ArtdaqComponent_hh__
#define __ArtdaqComponent_hh__

enum {
  kBoardReader   = 1,
  kEventBuilder  = 2,
  kDataLogger    = 3,
  kDispatcher    = 4,
                                        // anything else...
};

struct ArtdaqComponent_t {
  std::string   name;
  int           type;
  int           rank;
  int           xmlprc_port;
  int           target;
  std::string   xmlrpc_url;
  int           subsystem;
  int           n_fragment_types;
};

#endif
