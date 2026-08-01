#include "archive.hpp"
#include "../http/http.hpp"
#include "../providers/codeforces.hpp"

#include <iostream>
#include <string>

int archive_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "Archive commands\n";
    std::cout << "    update\n";
    std::cout << "    list\n";
    std::cout << "    install\n";
    return 0;
  }

  std::string subcommand = argv[0];

  if(subcommand == "update") {
    std::vector<Contest> contests = codeforces_get_contests();
    std::cout << "Found " << contests.size() << " contests\n\n";
    for(int i = 0; i < 10 && i < (int)contests.size(); i++) {
      std::cout << contests[i].id << " " << contests[i].name << "\n";
    }
  } else if(subcommand == "list") {
    std::cout << "Listing archive...\n";
  } else if(subcommand == "install") {
    std::cout << "Installing archive...\n";
  } else {
    std::cout << "Unknown archive command\n";
  }

  return 0;
}