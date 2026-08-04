#include "cli.hpp"
#include "commands/archive.hpp"
#include "commands/convert.hpp"
#include <iostream>
#include <string>

int CLI::run(int argc, char **argv) {
  if(argc == 1) {
    std::cout << "Local Contest Archive\n";
    std::cout << "Usage:\n";
    std::cout << "    lca archive\n";
    std::cout << "    lca convert\n";
    return 0;
  }

  std::string command = argv[1];

  if(command == "archive") {
    return archive_command(argc - 2, argv + 2);
  }
  if(command == "convert") {
    return convert_command(argc - 2, argv + 2);
  }

  std::cout << "Unknown command\n";
  return 1;
}