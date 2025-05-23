

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
