#include "archive.hpp"
#include "../http/http.hpp"

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
    std::string response = http_get("https://codeforces.com/api/contest.list");
    std::cout << response.substr(0, 500) << '\n';
  } else if(subcommand == "list") {
    std::cout << "Listing archive...\n";
  } else if(subcommand == "install") {
    std::cout << "Installing archive...\n";
  } else {
    std::cout << "Unknown archive command\n";
  }

  return 0;
}