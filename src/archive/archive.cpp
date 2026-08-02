#include "archive.hpp"
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <unordered_map>

using json = nlohmann::json;

std::filesystem::path archive_path() {
  char *home = std::getenv("HOME");

  if(home == nullptr) {
    throw std::runtime_error("HOME environment variable not found.");
  }

  return std::filesystem::path(home) / ".local" / "share" / "lca" / "archive";
}

void archive_save_codeforces(const std::vector<Contest> &contests, const std::vector<Problem> &problems) {
  std::filesystem::path provider_path = archive_path() / "codeforces";
  std::filesystem::create_directories(provider_path);

  std::unordered_map<int, std::vector<Problem>> problems_by_contest;
  for(const Problem &problem : problems) {
    problems_by_contest[problem.contest_id].push_back(problem);
  }

  for(const Contest &contest : contests) {
    json contest_json;
    contest_json["id"] = contest.id;
    contest_json["name"] = contest.name;
    contest_json["year"] = contest.year;

    std::filesystem::path contest_path = provider_path / std::to_string(contest.id);
    std::filesystem::create_directories(contest_path);

    for(const Problem &problem : problems_by_contest[contest.id]) {
      json problem_json;
      problem_json["id"] = problem.id;
      problem_json["name"] = problem.name;

      std::filesystem::path problem_path = contest_path / problem.id;
      std::filesystem::create_directories(problem_path);

      std::ofstream fout(problem_path / "problem.json");
      fout << problem_json.dump(4);
    }

    std::ofstream fout(contest_path / "contest.json");
    fout << contest_json.dump(4);
  }
}

std::vector<Contest> archive_load_codeforces() {
  std::vector<Contest> contests;

  std::filesystem::path provider_path = archive_path() / "codeforces";
  if(!std::filesystem::exists(provider_path)) {
    return contests;
  }

  for(const auto &entry : std::filesystem::directory_iterator(provider_path)) {
    if(!entry.is_directory()) {
      continue;
    }

    std::ifstream fin(entry.path() / "contest.json");
    if(!fin) {
      continue;
    }

    json contest_json;
    fin >> contest_json;

    Contest contest;
    contest.id = contest_json["id"];
    contest.name = contest_json["name"];
    contest.year = contest_json["year"];
    contests.push_back(contest);
  }

  std::sort(contests.begin(), contests.end(), [](const Contest &a, const Contest &b) {
    return a.id > b.id;
  });
  return contests;
}

void archive_save_codeforces_statement(const Problem &problem, const std::string &html) {
  std::filesystem::path path = archive_path() / "codeforces" / std::to_string(problem.contest_id) / problem.id / "statement.html";
  std::ofstream fout(path);
  fout << html;
}