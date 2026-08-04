#include "atcoder.hpp"
#include "../providers/atcoder.hpp"
#include "../archive/archive.hpp"
#include "convert.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>

using json = nlohmann::json;

namespace {
  std::unordered_set<std::string> load_seen_tasks(const std::filesystem::path &provider_path) {
    std::unordered_set<std::string> seen;
    if(!std::filesystem::exists(provider_path)) {
      return seen;
    }
    for(const auto &contest_entry : std::filesystem::directory_iterator(provider_path)) {
      if(!contest_entry.is_directory()) {
        continue;
      }
      for(const auto &problem_entry : std::filesystem::directory_iterator(contest_entry.path())) {
        if(!problem_entry.is_directory()) {
          continue;
        }
        std::filesystem::path json_path = problem_entry.path() / "problem.json";
        if(!std::filesystem::exists(json_path)) {
          continue;
        }
        std::ifstream fin(json_path);
        if(!fin) {
          continue;
        }
        json j;
        try {
          fin >> j;
        } catch(...) {
          continue;
        }
        if(j.contains("task")) {
          seen.insert(j["task"]);
        }
      }
    }
    return seen;
  }

  void save_contest_json(const std::filesystem::path &contest_path, const AtcoderContest &contest) {
    json j;
    j["id"] = contest.id;
    j["name"] = contest.name;
    j["year"] = 0;
    std::ofstream fout(contest_path / "contest.json");
    fout << j.dump(4);
  }

  void save_problem_json(const std::filesystem::path &problem_path, const AtcoderTask &task) {
    json j;
    j["id"] = task.letter;
    j["name"] = task.title;
    j["task"] = task.id;
    j["time_limit"] = task.time_limit;
    j["memory_limit"] = task.memory_limit;
    std::ofstream fout(problem_path / "problem.json");
    fout << j.dump(4);
  }

  void atcoder_update(const std::vector<std::string> &only_contests) {
    std::filesystem::path provider_path = archive_path() / "atcoder";
    std::filesystem::create_directories(provider_path);

    std::unordered_set<std::string> seen = load_seen_tasks(provider_path);

    std::vector<std::string> ids;
    if(!only_contests.empty()) {
      ids = only_contests;
    } else {
      std::cout << "Listing contests...\n";
      ids = atcoder_list_contests();
    }
    std::cout << "Contests to check: " << ids.size() << "\n";

    int total_new = 0, failed_contests = 0;
    for(const std::string &contest_id : ids) {
      AtcoderContest contest;
      try {
        contest = atcoder_fetch_contest(contest_id);
      } catch(const std::exception &e) {
        std::cerr << "Failed contest " << contest_id << ": " << e.what() << "\n";
        failed_contests++;
        continue;
      }
      if(contest.tasks.empty()) {
        continue;
      }

      std::vector<AtcoderTask> todo;
      for(const AtcoderTask &task : contest.tasks) {
        if(!seen.count(task.id)) {
          todo.push_back(task);
        }
      }
      if(todo.empty()) {
        continue;
      }

      std::filesystem::path contest_path = provider_path / contest_id;
      std::filesystem::create_directories(contest_path);

      int saved = 0;
      for(const AtcoderTask &task : todo) {
        std::filesystem::path problem_path = contest_path / task.letter;
        std::filesystem::create_directories(problem_path);

        if(!std::filesystem::exists(problem_path / "statement.html")) {
          try {
            std::string stmt = atcoder_build_statement_html(contest_id, task);
            std::ofstream fout(problem_path / "statement.html");
            fout << stmt;
            fout.close();
            if(!fout.good()) {
              continue;
            }
          } catch(const std::exception &e) {
            std::cerr << "Failed statement " << task.id << ": " << e.what() << "\n";
            continue;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        save_problem_json(problem_path, task);
        seen.insert(task.id);
        saved++;
      }

      if(saved > 0) {
        save_contest_json(contest_path, contest);
        total_new += saved;
        std::cout << contest_id << ": " << saved << " problems\n";
      }
    }

    std::cout << "Done. Saved " << total_new << " problems" <<
                 (failed_contests ? ", failed" + std::to_string(failed_contests) + " contests" : "") << ".\n";
  }

  void atcoder_list() {
    std::filesystem::path provider_path = archive_path() / "atcoder";
    if(!std::filesystem::exists(provider_path)) {
      std::cout << "No atcoder archive found.\n";
      return;
    }

    int total = 0;
    for(const auto &contest_entry : std::filesystem::directory_iterator(provider_path)) {
      if(!contest_entry.is_directory()) {
        continue;
      }
      std::ifstream fin(contest_entry.path() / "contest.json");
      if(!fin) {
        continue;
      }
      json j;
      fin >> j;

      int problems = 0;
      for(const auto &problem_entry : std::filesystem::directory_iterator(contest_entry.path())) {
        if(problem_entry.is_directory()) {
          problems++;
        }
      }
      std::cout << j["id"] << " " << j["name"] << " (" << problems << " problems)\n";
      total++;
    }
    std::cout << "Found " << total << " contests\n";
  }
}

int atcoder_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "AtCoder commands\n";
    std::cout << "    update [contest_ids]    download and extract contests (all if none given)\n";
    std::cout << "    list                    list available contests\n";
    std::cout << "    convert [contest_ids]   convert HTML statements to markdown from contests (all if none given)\n";
    return 0;
  }

  std::string subcommand = argv[0];

  if(subcommand == "update") {
    std::vector<std::string> ids(argv + 1, argv + argc);
    atcoder_update(ids);
  } else if(subcommand == "list") {
    atcoder_list();
  } else if(subcommand == "convert") {
    std::vector<std::string> ids(argv + 1, argv + argc);
    return convert_provider(archive_path() / "atcoder", ids);
  } else {
    std::cout << "Unknown atcoder command\n";
    return 1;
  }

  return 0;
}