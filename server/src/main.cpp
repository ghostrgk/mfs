#include <iostream>
#include <string>

//#include <fs++/filesystem.h>
#include <regex>

int main() {
  std::string query;
  std::getline(std::cin, query);
  static const std::regex full_regex("^(/|((/\\w+)+))$");
//  static const std::regex full_regex(R"(^\\s*m\\s+(/|((\/\\w+)+))\\s*$)");
  std::cerr << "(query=" << query << ") " << query.size() << ' ';

  std::smatch match;
  if (!std::regex_match(query, match, full_regex)) {
    std::cout << "Wrong path format" << std::endl;
  } else {
    std::cout << "Ok" << std::endl;
  }

  return 0;
}