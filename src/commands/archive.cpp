#include "archive.hpp"
#include "../http/http.hpp"
#include "../providers/codeforces.hpp"
#include "../archive/archive.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>

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

  std::filesystem::path job_file = std::filesystem::temp_directory_path() / "lca_jobs.txt";
  std::ofstream fout(job_file);
  for(const Problem &problem : problems) {
    std::filesystem::path statement_path = archive_statement_path("codeforces", problem);
    std::filesystem::create_directories(statement_path.parent_path());
    fout << codeforces_problem_url(problem) << '\t' << statement_path.string() << '\n';
  }

  std::cout << "Downloading " << problems.size() << " statements...\n";
  int result = codeforces_download_statements(job_file);
  std::filesystem::remove(job_file);

  if(result != 0) {
    throw std::runtime_error("Some statements failed to download.");
  }
  std::cout << "Done\n";
}