#include <iostream>
#include <iterator>
#include <regex>
#include <string>
 
int test_regex() {
  
  std::string s = "Some people, when confronted with a problem, think "
    "\"I know, I'll use regular expressions.\" "
    "Now they have two problems.";
  
  std::regex self_regex("REGULAR EXPRESSIONS",
                        std::regex_constants::ECMAScript | std::regex_constants::icase);
  if (std::regex_search(s, self_regex))
    std::cout << "Text contains the phrase 'regular expressions'\n";
  
  std::regex word_regex("(\\w+)");
  auto words_begin = 
    std::sregex_iterator(s.begin(), s.end(), word_regex);
  auto words_end = std::sregex_iterator();
  
  std::cout << "Found "
            << std::distance(words_begin, words_end)
            << " words\n";

  // parse out mu2edaq22-ctrl
  
  std::string s1 = "http://mu2edaq22-ctrl.fnal.gov:6600/RPC2";
  std::regex pattern(R"(^http://)");
  
  std::string stripped_text = std::regex_replace(s1, pattern, "");

  std::cout << "stripped:" << stripped_text << std::endl;

  std::regex pat2(R"(\.fnal\.gov+$)");
  std::string stripped_text2 = std::regex_replace(stripped_text, pat2, "");

  std::cout << "stripped2:" << stripped_text2 << std::endl;
  return 0;
}
