///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
#include <string>
#include "tfm/my_xmlrpc.hh"

//-----------------------------------------------------------------------------
int main(int argc, char** argv) {

  int nw(-1);

  std::string exec_name = "none";

  const char*       words[100];
  std::string w    [100];

  words[0] = exec_name.data();  // executable name, not used

  if (argc == 1) {
    printf("ERROR : just one parameter\n");
    return -1;
  }

  std::string cmd = argv[1];

  if (cmd == "status") {
                                        // status

    w[0] = "http://localhost:18000/RPC2";
    w[1] = "state";
    w[2] = "daqint";

    words[1] = w[0].data();
    words[2] = w[1].data();
    words[3] = w[2].data();

    nw = 4;
  }
  else if (cmd == "config") {
    std::string run_number = argv[2];

    w[0] = "http://localhost:18000/RPC2";
    w[1] = "state_change";
    w[2] = "daqint"; 
    w[3] = "configuring"; 
    w[4] = "struct/{run_number:i/"+run_number+"}";

    words[1] = w[0].data();
    words[2] = w[1].data();
    words[3] = w[2].data();
    words[4] = w[3].data();
    words[5] = w[4].data();

    nw = 6;
  }
  else if (cmd == "start") {

    w[0] = "http://localhost:18000/RPC2";
    w[1] = "state_change";
    w[2] = "daqint"; 
    w[3] = "starting"; 
    w[4] = "struct/{ignored_variable:i/999}";

    words[1] = w[0].data();
    words[2] = w[1].data();
    words[3] = w[2].data();
    words[4] = w[3].data();
    words[5] = w[4].data();

    nw = 6;
  }
  else if (cmd == "stop") {

    w[0] = "http://localhost:18000/RPC2";
    w[1] = "state_change";
    w[2] = "daqint"; 
    w[3] = "starting"; 
    w[4] = "struct/{ignored_variable:i/999}";

    words[1] = w[0].data();
    words[2] = w[1].data();
    words[3] = w[2].data();
    words[4] = w[3].data();
    words[5] = w[4].data();

    nw = 6;
  }

  std::string res;

  my_xmlrpc(nw, words, res); // http://localhost:$port/RPC2 state daqint;
}
