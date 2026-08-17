#include "cli.hpp"
#include "commands/codeforces.hpp"
#include "commands/atcoder.hpp"
#include "commands/convert.hpp"
#include "commands/oi.hpp"
#include "commands/work.hpp"
#include <iostream>
#include <string>

int CLI::run(int argc, char **argv) {
  if(argc == 1) {
    std::cout << "Local Contest Archive\n";
    std::cout << "Usage:\n";
    std::cout << "    lca codeforces\n";
    std::cout << "    lca atcoder\n";
    std::cout << "    lca oi\n";
    std::cout << "    lca work\n";
    std::cout << "    lca continue\n";
    std::cout << "    lca solved\n";
    std::cout << "    lca unsolved\n";
    std::cout << "    lca unwork\n";
    std::cout << "    lca bookmark\n";
    std::cout << "    lca unbookmark\n";
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
  if(command == "work") {
    return work_command(argc - 2, argv + 2);
  }
  if(command == "continue") {
    return work_continue_command(argc - 2, argv + 2);
  }
  if(command == "solved") {
    return work_solved_command(argc - 2, argv + 2);
  }
  if(command == "unsolved") {
    return work_unsolved_command(argc - 2, argv + 2);
  }
  if(command == "unwork") {
    return work_unwork_command(argc - 2, argv + 2);
  }
  if(command == "bookmark") {
    return work_bookmark_command(argc - 2, argv + 2);
  }
  if(command == "unbookmark") {
    return work_unbookmark_command(argc - 2, argv + 2);
  }
  if(command == "convert") {
    return convert_command();
  }

  std::cout << "Unknown command\n";
  return 1;
}