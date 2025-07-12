///////////////////////////////////////////////////////////////////////////////
// test_popen("artdaq_xmlrpc","dr_metrics","mu2edaq09:21301")
// test_popen("monitor_host","disk_io","/data/tracker/vst/mu2e_daquser_001")
///////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#include <string>
#include <ranges>
#include <format>
#include <cstdio>                   // For popen and pclose
#include "nlohmann/json.hpp"
using nlohmann::json;

#include <ranges> // Requires C++20

std::vector<std::string> split_string(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    for (const auto& subrange : s | std::views::split(delimiter)) {
        tokens.push_back(std::string(subrange.begin(), subrange.end()));
    }
    return tokens;
}

std::string popen_shell_command(const std::string& command) {
  char buffer[4096];                         // Buffer to store command output
  std::string result = "";                  // String to accumulate the entire output
  FILE* pipe = popen(command.c_str(), "r"); // Open a pipe to the command
  if (!pipe) {
    return "Error: popen failed!";          // Handle error if pipe creation fails
  }
  try {
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
      result += buffer;                     // Append buffer content to the result string
    }
  } catch (...) {
    pclose(pipe);                           // Ensure pipe is closed even if an exception occurs
    throw;
  }
  pclose(pipe);                             // Close the pipe
  return result;                            // Return the captured output
}

//-----------------------------------------------------------------------------
// Host: "mu2edaq22-ctrl" or "mu2edaq09"
// Backend: "artdaq_xmlrpc" or "monitor_host"
// Dir: output directory for "monitor_host" , or Host:Port for test_xmlrpc
//-----------------------------------------------------------------------------
std::string test_popen(const std::string& Backend, const std::string& Job, const char* Params, int DiagLevel=0) {
  std::string cmd;
  
  if (Backend == "artdaq_xmlrpc") {
    std::vector<std::string>  w = split_string(Params,":");
    std::string host = w[0];
    std::string port = w[1];
    std::cout << "host:" << host << " port:" << port << std::endl;
    
    cmd = std::format("python config/scripts/artdaq_xmlrpc.py --test={:s} --host={} --port={} --diag_level={:d}",
                      Job,host,port,DiagLevel);
  }
  else if (Backend == "monitor_host") {
    std::string dir = Params;
    std::cout << "dir:" << dir << std::endl;
    cmd = std::format("python config/scripts/monitor_host.py --job={} --dir={} --diag_level={}",
                      Job,dir,DiagLevel); 
  }


  std::string output  = popen_shell_command(cmd);
  std::cout << "Command Output:\n" << output; //  << std::endl;

  // std::ifstream ifs(output);

  // std::string line;
  // while (getline (ifs, line)) {
  //   std::cout << line << std::endl;
  // }

  // ifs.close();
  
  json jf = json::parse(output);

  std::cout << "parsed json output:\n" << jf << std::endl;

    // command = "echo Hello from C++!"; // Another example
    // output = executeCommandAndGetOutput(command);
    // std::cout << "Command Output:\n" << output << std::endl;

  return std::string("ok"); // output;
}
