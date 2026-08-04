#include "codeforces.hpp"
#include "../http/http.hpp"
#include "../providers/codeforces.hpp"
#include "../archive/archive.hpp"
#include "convert.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace {
  bool codeforces_statement_exists(const Problem &problem) {
    std::filesystem::path problem_path = archive_path() / "codeforces" / std::to_string(problem.contest_id) / problem.id;
    return std::filesystem::exists(problem_path / "statement.html") || std::filesystem::exists(problem_path / "statement.pdf");
  }

  void codeforces_list() {
    std::vector<Contest> contests = archive_load_codeforces();
    std::cout << "Found " << contests.size() << " contests\n";
    for(const Contest &contest : contests) {
      std::cout << contest.id << " " << contest.name << "\n";
    }
  }
}

void codeforces_update(const std::string &only_contest) {
  std::vector<Contest> contests = codeforces_get_contests();
  std::vector<Problem> problems = codeforces_get_problems();

  if(!only_contest.empty()) {
    int contest_id = 0;
    try {
      contest_id = std::stoi(only_contest);
    } catch(...) {
      std::cout << "Invalid contest id: " << only_contest << "\n";
      return;
    }
    contests.erase(std::remove_if(contests.begin(), contests.end(),
      [contest_id](const Contest &c) { return c.id != contest_id; }), contests.end());
    problems.erase(std::remove_if(problems.begin(), problems.end(),
      [contest_id](const Problem &p) { return p.contest_id != contest_id; }), problems.end());
  }

  archive_save_codeforces(contests, problems);

  std::filesystem::path job_file = std::filesystem::temp_directory_path() / "lca_jobs.txt";
  std::ofstream fout(job_file);
  int missing = 0;
  for(const Problem &problem : problems) {
    if(codeforces_statement_exists(problem)) {
      continue;
    }

    std::filesystem::path statement_path = archive_statement_path("codeforces", problem);
    std::filesystem::create_directories(statement_path.parent_path());
    fout << codeforces_problem_url(problem) << '\t' << statement_path.string() << '\n';
    missing++;
  }
  fout.close();

  std::cout << "Downloading " << missing << "statements...\n";
  int result = codeforces_download_statements(job_file);
  std::filesystem::remove(job_file);

  if(result != 0) {
    throw std::runtime_error("Some statements failed to download.");
  }
  std::cout << "Done\n";
}

int codeforces_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "Codeforces commands\n";
    std::cout << "    update [contest_id]\n";
    std::cout << "    list\n";
    std::cout << "    convert [contest_id]\n";
    return 0;
  }

  std::string subcommand = argv[0];

  if(subcommand == "update") {
    std::string only_contest;
    if(argc >= 2) {
      only_contest = argv[1];
    }
    codeforces_update(only_contest);
  } else if(subcommand == "list") {
    codeforces_list();
  } else if(subcommand == "convert") {
    std::string only_contest;
    if(argc >= 2) {
      only_contest = argv[1];
    }
    return convert_provider(archive_path() / "codeforces", only_contest);
  } else {
    std::cout << "Unknown codeforces command\n";
    return 1;
  }

  return 0;
}