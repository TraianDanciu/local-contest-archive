#include "cli.hpp"
#include "commands/codeforces.hpp"
#include "commands/atcoder.hpp"
#include "commands/convert.hpp"
#include "commands/oi.hpp"
#include <iostream>
#include <string>

int CLI::run(int argc, char **argv) {
  if(argc == 1) {
    std::cout << "Local Contest Archive\n";
    std::cout << "Usage:\n";
    std::cout << "    lca codeforces\n";
    std::cout << "    lca atcoder\n";
    std::cout << "    lca oi\n";
    std::cout << "    lca convert\n";
    return 0;
  }

  std::string command = argv[1];

  if(command == "codeforces") {
    return codeforces_command(argc - 2, argv + 2);
  }
  if(command == "atcoder") {
    return atcoder_command(argc - 2, argv + 2);
  }
  if(command == "oi") {
    return oi_command(argc - 2, argv + 2);
  }
  if(command == "convert") {
    return convert_command();
  }

  std::cout << "Unknown command\n";
  return 1;
}