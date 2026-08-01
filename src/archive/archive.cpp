#include "archive.hpp"
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::filesystem::path archive_path() {
  char *home = std::getenv("HOME");

  if(home == nullptr) {
    throw std::runtime_error("HOME environment variable not found.");
  }

  return std::filesystem::path(home) / ".local" / "share" / "lca" / "archive";
}

void archive_save_codeforces(const std::vector<Contest> &contests) {
  std::filesystem::path provider_path = archive_path() / "codeforces";
  std::filesystem::create_directories(provider_path);

  json contests_json = json::array();
  for(const Contest &contest : contests) {
    json contest_json;
    contest_json["id"] = contest.id;
    contest_json["name"] = contest.name;
    contest_json["year"] = contest.year;
    contests_json.push_back(contest_json);

    std::filesystem::path contest_path = provider_path / std::to_string(contest.id);
    std::filesystem::create_directories(contest_path);

    std::ofstream fout(contest_path / "contest.json");
    fout << contest_json.dump(4);
  }

  std::ofstream fout(provider_path / "contests.json");
  fout << contests_json.dump(4);
}

std::vector<Contest> archive_load_codeforces() {
  return {};
}