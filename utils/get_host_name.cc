//

#include <string>
#include <cstring>
#include "frontends/utils/utils.hh"

//-----------------------------------------------------------------------------
// assume Hostname is either fully domain-qualified name, or 'local'
//-----------------------------------------------------------------------------
std::string get_full_host_name(const char* Hostname) {

  std::string name;
  char buf[100];

  if (strstr(Hostname,"local") != Hostname) {
//-----------------------------------------------------------------------------
// assume the Hostname is already fully qualified with the domain name
//-----------------------------------------------------------------------------
    name = Hostname;
  }
  else {
//-----------------------------------------------------------------------------
// assume "local" or "localhost", replace it with the fully qualified name
//-----------------------------------------------------------------------------
    FILE* pipe = popen("hostname -f", "r");
    while (!feof(pipe)) {
      char* s = fgets(buf, 100, pipe);
      if (s) {
        int len = strlen(s);
        if ((len > 0) and (s[len-1] == '\n')) s[len-1] = 0;
        if (s[0] != 0) name += buf;
      }
    }
    pclose(pipe);
  }
  return name;
}

//-----------------------------------------------------------------------------
// assume Hostname is either fully domain-qualified name, or 'local'
//-----------------------------------------------------------------------------
std::string get_short_host_name(const char* Hostname) {

  std::string name;
  char buf[100];

  if (strstr(Hostname,"local") != Hostname) {
//-----------------------------------------------------------------------------
// assume the Hostname is already fully qualified with the domain name
//-----------------------------------------------------------------------------
    name = Hostname;
  }
  else {
//-----------------------------------------------------------------------------
// assume "local" or "localhost", replace it with the fully qualified name
//-----------------------------------------------------------------------------
    FILE* pipe = popen("hostname -s", "r");
    while (!feof(pipe)) {
      char* s = fgets(buf, 100, pipe);
      if (s) {
        int len = strlen(s);
        if ((len > 0) and (s[len-1] == '\n')) s[len-1] = 0;
        if (s[0] != 0) name += buf;
      }
    }
    pclose(pipe);
  }
  return name;
}

