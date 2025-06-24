#include <iostream>
#include <string>
#include <cstdio> // For popen and pclose
#include "nlohmann/json.hpp"

std::string executeCommandAndGetOutput(const std::string& command) {
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
int test_popen(const char* Test, const char* Host, int Port=21000) {
  
  std::string command = std::format("python config/scripts/test_xmlrpc.py --test={:s} --host={:s} --port={:d}",
                                    Test,Host,Port); // Example command to execute
  std::string output  = executeCommandAndGetOutput(command);
  std::cout << "Command Output:\n" << output; //  << std::endl;

  std::ifstream ifs(output);
  nlohmann::json jf = nlohmann::json::parse(ifs);
  

    // command = "echo Hello from C++!"; // Another example
    // output = executeCommandAndGetOutput(command);
    // std::cout << "Command Output:\n" << output << std::endl;

  return 0;
}
