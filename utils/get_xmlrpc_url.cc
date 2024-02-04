//

#include "TRACE/tracemf.h"
#define TRACE_NAME "get_xml_rpc_url"

#include "midas.h"

//-----------------------------------------------------------------------------
int get_xmlrpc_url(HNDLE& hDB, HNDLE& hArtdaqNodeKey, int Partition, const char* ComponentName, std::string& Url) {

  char  hostname[50];
  int   sz = sizeof(hostname);
  db_get_value(hDB,hArtdaqNodeKey,"Hostname", hostname, &sz, TID_STRING, TRUE);
  TLOG(TLVL_INFO+10) << "001 hostname:" << hostname << " ComponentName:" << ComponentName;

  HNDLE hComponentKey; 
  try {
    db_find_key(hDB,hArtdaqNodeKey,ComponentName,&hComponentKey);
  }
  catch (...) {
    TLOG(TLVL_ERROR) << "001.1: failed to get the ARTDAQ component " << ComponentName << " key";
  }
  

  TLOG(TLVL_INFO+10) << "001.2 hostname:" << hostname << " ComponentName:" << ComponentName;
//-----------------------------------------------------------------------------
// if port is defined explicitly, use that
// otherwise, the port number is defined by the executable rank 
//-----------------------------------------------------------------------------
  int   port_number;
  HNDLE hPortKey; 
  sz = sizeof(int);
  if (db_find_key(hDB, hComponentKey, "XmlrpcPort", &hPortKey) == DB_SUCCESS) {
    db_get_value(hDB, hComponentKey, "XmlrpcPort", &port_number, &sz, TID_INT32, FALSE);
  }
  else {
    int rank;
    db_get_value(hDB, hComponentKey, "Rank", &rank, &sz, TID_INT32, FALSE);
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
  
  if (strcmp(hostname,local_hostname.data()) == 0) strcpy(hostname,"localhost");

  Url = "http://"+std::string(hostname)+":"+std::to_string(port_number)+"/RPC2";

  TLOG(TLVL_INFO+10) << "003 _brUrl:" << Url;
  return 0;
}

