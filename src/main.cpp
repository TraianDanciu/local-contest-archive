#include "cli/cli.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
  try {
    CLI cli;
    return cli.run(argc, argv);
  } catch(const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }
}