///////////////////////////////////////////////////////////////////////////////
// P.M. need names on the data network  "*-data'
// the host names just to be encoded into the DTC names
///////////////////////////////////////////////////////////////////////////////
#include <string>
#include <cstring>
#include "utils/utils.hh"

//-----------------------------------------------------------------------------
// get fully domain-qualified host name on a given subnet
// make sure 'ipmi' names are not used
//-----------------------------------------------------------------------------
std::string get_full_host_name(const char* Subnet) {

  std::string name;
  char cmd[256], buf[128];

  sprintf(cmd,"ifconfig -a | grep %s | awk '{print $2}' | nslookup | grep -v ipmi | head -n 1 | sed 's/\\.$//' | awk '{print $NF}'",
          Subnet);
//-----------------------------------------------------------------------------
// the output is one line max
//-----------------------------------------------------------------------------
  FILE* pipe = popen(cmd, "r");
  char* s = fgets(buf, 128, pipe);
  if (s) {
    int len = strlen(s);
    if ((len > 0) and (s[len-1] == '\n')) s[len-1] = 0;
    name = buf;
  }
  pclose(pipe);
  
  return name;
}

//-----------------------------------------------------------------------------
// get short host name on a given subnet
//-----------------------------------------------------------------------------
std::string get_short_host_name(const char* Subnet) {

  std::string name;
  char cmd[256], buf[128];

  if (Subnet[0] == 0) {
//-----------------------------------------------------------------------------
// subnet is undefined , return hostname - that would normally give the name on a public subnet
// which is a needed 'label'
//-----------------------------------------------------------------------------
    sprintf(cmd,"hostname -s");
  }
  else {
    sprintf(cmd,
            "ifconfig -a | grep %s | awk '{print $2}' | nslookup | head -n 1 | sed 's/\\.$//' | awk '{print $NF}' | awk -F . '{print $1}'",
            Subnet);
  }
//-----------------------------------------------------------------------------
// the output is one line max
//-----------------------------------------------------------------------------
  FILE* pipe = popen(cmd, "r");
  char* s    = fgets(buf, 128, pipe);
  if (s) {
    int len = strlen(s);
    if ((len > 0) and (s[len-1] == '\n')) s[len-1] = 0;
    name = buf;
  }
  pclose(pipe);

  return name;
}

