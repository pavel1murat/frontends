//------------------------------------------------------------------------------
// P.Murat: utility functions
//-----------------------------------------------------------------------------
#ifndef __frontends_utils_hh__
#define __frontends_utils_hh__

#include <string>

#include "midas.h"

//-----------------------------------------------------------------------------
// global variables - bad, but no way around without changing the interfaces
// make the name ugly in hope to get rid of this 
// that would require making DEVICE_DRIVER a C++ class, rather than a struct
//-----------------------------------------------------------------------------
namespace FrontendsGlobals {
  extern DEVICE_DRIVER* _driver;
};

//-----------------------------------------------------------------------------
// for a given artdaq node and a component name, return XMLRPC url
//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqNode, int Partition, const char* Component, std::string& Url);

//-----------------------------------------------------------------------------
// for 'Hostname' : "aaaaa.fnal.gov", returns "aaaaa"
//-----------------------------------------------------------------------------
std::string short_nodename(const char* Hostname);

#endif
