//

#include <string>
#include <cstring>
#include "frontends/utils/utils.hh"

//-----------------------------------------------------------------------------
// 'Hostname' : "aaaaa.fnal.gov"
// return     : "aaaaa"
//-----------------------------------------------------------------------------
std::string short_nodename(const char* Hostname) {
  std::string node;
  int   len = strlen(Hostname);
  const char* ind = index(Hostname,'.');
  if (ind == nullptr) {
    node = Hostname;
  }
  else {
    len = ind-Hostname;
    node.resize(len);
    strncpy(node.data(),Hostname,len);
  }
  return node;
}

