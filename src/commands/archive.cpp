#include "archive.hpp"
#include "../http/http.hpp"
#include "../providers/codeforces.hpp"
#include "../archive/archive.hpp"

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
    archive_update();
  } else if(subcommand == "list") {
    std::vector<Contest> contests = archive_load_codeforces();
    std::cout << "Found " << contests.size() << " contests\n";
    for(const Contest &contest : contests) {
      std::cout << contest.id << " " << contest.name << "\n";
    }
  } else if(subcommand == "install") {
    std::cout << "Installing archive...\n";
  } else {
    std::cout << "Unknown archive command\n";
  }

  return 0;
}

bool archive_statement_exists(const std::string &provider, const Problem &problem) {
  std::filesystem::path path = archive_path() / provider / std::to_string(problem.contest_id) / problem.id / "statement.html";
  return std::filesystem::exists(path);
}

void archive_update() {
  std::vector<Contest> contests = codeforces_get_contests();
  std::vector<Problem> problems = codeforces_get_problems();
  archive_save_codeforces(contests, problems);

  std::cout << "Downloading statements\n";
  int downloaded = 0;
  for(const Problem &problem : problems) {
    try {
      std::filesystem::path statement_path = archive_statement_path("codeforces", problem);
      std::filesystem::create_directories(statement_path.parent_path());
      codeforces_download_statement(problem, statement_path);
    } catch(const std::exception &e) {
      std::cerr << "\nFailed to download " << problem.id << ": " << e.what() << '\n';
    }

    downloaded++;
    std::cout << "\rDownloaded " << downloaded << "/" << problems.size() << " statements..." << std::flush;
  }

  std::cout << "\nDone\n";
}