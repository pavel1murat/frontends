//

#include "TRACE/tracemf.h"
#define TRACE_NAME "get_xml_rpc_url"

#include "midas.h"

//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqConf, const char* Hostname, int Partition, const char* ComponentName, std::string& Url) {

  TLOG(TLVL_INFO+10) << "001 Hostname:" << Hostname << " ComponentName:" << ComponentName;

  HNDLE h_component; 
  try {
    db_find_key(hDB,hArtdaqConf,ComponentName,&h_component);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "001.1: failed to get the ARTDAQ component " << ComponentName << " key";
  }
  

  TLOG(TLVL_INFO+10) << "001.2 Hostname:" << Hostname << " ComponentName:" << ComponentName;
//-----------------------------------------------------------------------------
// if port is defined explicitly, use that
// otherwise, the port number is defined by the executable rank 
//-----------------------------------------------------------------------------
  int   port_number;
  HNDLE hPortKey; 
  int sz = sizeof(int);
  if (db_find_key(hDB, h_component, "XmlrpcPort", &hPortKey) == DB_SUCCESS) {
    db_get_value(hDB, h_component, "XmlrpcPort", &port_number, &sz, TID_INT32, FALSE);
  }
  else {
    int rank;
    db_get_value(hDB, h_component, "Rank", &rank, &sz, TID_INT32, FALSE);
    port_number = 10000+1000*Partition+100+rank;
  }

  TLOG(TLVL_INFO+10) << "001.3 port_number:" << port_number;

  std::string local_hostname;
  char buf[100];
  FILE* pipe = popen("hostname -f", "r");
  while (!feof(pipe)) {
    char* s = fgets(buf, 100, pipe);
    if (s) local_hostname += buf;
  }
  pclose(pipe);

  TLOG(TLVL_INFO+10) << "002.1 local_hostname:" << local_hostname;
  
  std::string hostname = Hostname;
  if (strcmp(Hostname,local_hostname.data()) == 0) hostname = "localhost";

  Url = "http://"+hostname+":"+std::to_string(port_number)+"/RPC2";

  TLOG(TLVL_INFO+10) << "003 Url:" << Url;
  return 0;
}

