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
#include <unordered_set>

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

  void codeforces_update(const std::vector<std::string> &only_contests) {
    std::vector<Contest> contests = codeforces_get_contests();
    std::vector<Problem> problems = codeforces_get_problems();

    if(!only_contests.empty()) {
      std::unordered_set<int> wanted;
      for(const std::string &id : only_contests) {
        try {
          wanted.insert(std::stoi(id));
        } catch(...) {
          std::cout << "Invalid contest id: " << id << "\n";
          return;
        }
      }
      contests.erase(std::remove_if(contests.begin(), contests.end(),
        [&wanted](const Contest &c) { return !wanted.count(c.id); }), contests.end());
      problems.erase(std::remove_if(problems.begin(), problems.end(),
        [&wanted](const Problem &p) { return !wanted.count(p.contest_id); }), problems.end());
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
}

int codeforces_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "Codeforces commands\n";
    std::cout << "    update [contest_id ...]    download and extract contests (all if none given)\n";
    std::cout << "    list                       list available contests\n";
    std::cout << "    convert [contest_id ...]   convert HTML statements to markdown from contests (all if none given)\n";
    return 0;
  }

  std::string subcommand = argv[0];

  if(subcommand == "update") {
    std::vector<std::string> ids(argv + 1, argv + argc);
    codeforces_update(ids);
  } else if(subcommand == "list") {
    codeforces_list();
  } else if(subcommand == "convert") {
    std::vector<std::string> ids(argv + 1, argv + argc);
    return convert_provider(archive_path() / "codeforces", ids);
  } else {
    std::cout << "Unknown codeforces command\n";
    return 1;
  }

  return 0;
}