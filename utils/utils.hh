//------------------------------------------------------------------------------
// P.Murat: utility functions
//-----------------------------------------------------------------------------
#ifndef __frontends_utils_hh__
#define __frontends_utils_hh__

#include <string>
#include <vector>

#include "midas.h"
//-----------------------------------------------------------------------------
// global variables - bad, but no way around without changing the interfaces
// make the name ugly in hope to get rid of this 
// that would require making DEVICE_DRIVER a C++ class, rather than a struct
//-----------------------------------------------------------------------------
namespace FrontendsGlobals {
  extern DEVICE_DRIVER* _driver;
};

std::string expand_env_vars    (const std::string& String);
std::vector<std::string> splitString(const std::string& s, char delimiter);
//-----------------------------------------------------------------------------
// hostname is defined by a subnet, i.e. 192.168.1.107 and nslookup 
//-----------------------------------------------------------------------------
std::string get_full_host_name (const char* Subnet);
std::string get_short_host_name(const char* Subnet);
//-----------------------------------------------------------------------------
// for a given artdaq node and a component name, return XMLRPC url
//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqNode, const char* Hostname, int Partition, const char* Component, std::string& Url);

//-----------------------------------------------------------------------------
// execute shell command and get its stdout printout
//-----------------------------------------------------------------------------
std::string popen_shell_command(const std::string& command);

#endif
