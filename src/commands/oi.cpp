#include "oi.hpp"
#include "../providers/oi.hpp"
#include <iostream>
#include <string>
#include <vector>

void oi_list() {
  std::cout << "OI olympiads\n";
  for(const OiOlympiad &olympiad : oi_olympiads()) {
    std::cout << "    " << olympiad.name << "  " << olympiad.display << "\n";
  }
}

void oi_list_olympiad(const std::string &name) {
  const OiOlympiad *olympiad = oi_find_olympiad(name);
  if(olympiad == nullptr) {
    std::cout << "Unknown olympiad: " << name << "\n";
    return;
  }

  std::vector<OiYear> years;
  try {
    years = oi_list_years(*olympiad);
  } catch(const std::exception &e) {
    std::cout << "Failed to fetch database: " << e.what() << "\n";
    return;
  }

  std::cout << olympiad->name << " (" << olympiad->display << "): "
            << years.size() << " years\n";
  long long total = 0;
  for(const OiYear &year : years) {
    std::cout << "    " << year.year
              << "  " << (double)year.size / (1024.0 * 1024.0) << " MB\n";
    total += year.size;
  }
  std::cout << "Total: " << (double)total / (1024.0 * 1024.0) << " MB\n";
}

int oi_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "OI commands\n";
    std::cout << "    list [olympiad]                list olympiads and available years\n";
    std::cout << "    update <olympiad> [years...]   download and extract years (all if none given)\n";
    std::cout << "    status                         show downloaded olympiad years\n";
    return 0;
  }

  std::string subcommand = argv[0];

  if(subcommand == "list") {
    if(argc >= 2) {
      oi_list_olympiad(argv[1]);
    } else {
      oi_list();
    }
    return 0;
  }

  if(subcommand == "status") {
    return oi_status();
  }

  if(subcommand == "update") {
    if(argc < 2) {
      std::cout << "Usage: lca oi update <olympiad> [years...]\n";
      return 1;
    }

    const OiOlympiad *olympiad = oi_find_olympiad(argv[1]);
    if(olympiad == nullptr) {
      std::cout << "Unknown olympiad: " << argv[1] << "\n";
      std::cout << "Known olympiads:";
      for(const OiOlympiad &entry : oi_olympiads()) {
        std::cout << " " << entry.name;
      }
      std::cout << "\n";
      return 1;
    }

    std::vector<std::string> years(argv + 2, argv + argc);
    try {
      return oi_update(*olympiad, years);
    } catch(const std::exception &e) {
      std::cout << "Failed to fetch database: " << e.what() << "\n";
      return 1;
    }
  }

  std::cout << "Unknown oi command\n";
  return 1;
}
