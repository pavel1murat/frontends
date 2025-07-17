

#include "utils/utils.hh"
#include <string>
#include <regex>


namespace FrontendsGlobals {
  DEVICE_DRIVER* _driver;
}

// Update the input string.
void autoExpandEnvironmentVariables( std::string & text ) {
    static std::regex env( "\\$\\{([^}]+)\\}" );
    std::smatch match;
    while ( std::regex_search( text, match, env ) ) {
        const char * s = getenv( match[1].str().c_str() );
        const std::string var( s == NULL ? "" : s );
        text.replace( match[0].first, match[0].second, var );
    }
}

// Leave input alone and return new string.
std::string expand_env_vars(const std::string& input) {
  std::string text = input;
  autoExpandEnvironmentVariables(text);
  return text;
}


//-----------------------------------------------------------------------------
std::string popen_shell_command(const std::string& command) {
  char buffer[128];                         // Buffer to store command output
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
std::vector<std::string> splitString(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

