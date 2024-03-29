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
// assume Hostname is either a fully domain-qualified name, or 'local'
//-----------------------------------------------------------------------------
std::string get_full_host_name (const char* Hostname);
std::string get_short_host_name(const char* Hostname);

//-----------------------------------------------------------------------------
// for a given artdaq node and a component name, return XMLRPC url
//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqNode, const char* Hostname, int Partition, const char* Component, std::string& Url);

#endif
