#include "work.hpp"
#include "../archive/archive.hpp"
#include "../providers/oi.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
  std::string to_lower(std::string s) {
    for(char &c : s) {
      c = (char)std::tolower((unsigned char)c);
    }
    return s;
  }

  std::string to_upper(std::string s) {
    for(char &c : s) {
      c = (char)std::toupper((unsigned char)c);
    }
    return s;
  }

  fs::path progress_path() {
    const char *home = std::getenv("HOME");
    if(home == nullptr) {
      throw std::runtime_error("HOME environment variable not found.");
    }
    return fs::path(home) / ".local" / "share" / "lca" / "progress.json";
  }

  struct ProblemId {
    std::string provider;
    std::string contest;
    std::string letter;
    std::string path_str;
  };

  bool is_oi_olympiad(const std::string &name) {
    return oi_find_olympiad(name) != nullptr;
  }

  bool parse_problem_id(const std::string &input, ProblemId &out, std::string &error) {
    std::vector<std::string> parts;
    std::string current;
    for(char c : input) {
      if(c == '/') {
        parts.push_back(current);
        current.clear();
      } else {
        current.push_back(c);
      }
    }
    parts.push_back(current);

    if(parts.size() != 3 || parts[0].empty() || parts[1].empty() || parts[2].empty()) {
      error = "Expected <provider>/<contest>/<letter>, got: " + input;
      return false;
    }

    std::string provider = to_lower(parts[0]);
    if(provider != "codeforces" && provider != "atcoder" && !is_oi_olympiad(provider)) {
      error = "Unknown provider: " + parts[0];
      return false;
    }

    std::string letter = to_upper(parts[2]);
    if(letter.size() != 1 || !std::isalpha((unsigned char)letter[0])) {
      error = "Invalid problem letter: " + parts[2];
      return false;
    }

    out.provider = provider;
    out.contest = parts[1];
    out.letter = letter;
    out.path_str = provider + "/" + parts[1] + "/" + letter;
    return true;
  }

  fs::path problem_dir(const ProblemId &id) {
    return archive_path() / id.provider / id.contest / id.letter;
  }

  bool has_statement(const fs::path &dir) {
    for(const char *name : {"statement.pdf", "statement.html", "statement.md"}) {
      if(fs::exists(dir / name)) {
        return true;
      }
    }
    return false;
  }

  bool check_problem(const ProblemId &id) {
    fs::path dir = problem_dir(id);
    if(!fs::is_directory(dir)) {
      std::cout << "Problem not found locally: " << id.path_str << "\n";
      if(id.provider == "codeforces") {
        std::cout << "    Download it with: lca codeforces update " << id.contest << "\n";
      } else if(id.provider == "atcoder") {
        std::cout << "    Download it with: lca atcoder update " << id.contest << "\n";
      } else {
        std::cout << "    Download it with: lca oi update " << id.provider << " " << id.contest << "\n";
      }
      return false;
    }
    if(!has_statement(dir)) {
      std::cout << "Warning: no statement found in " << dir.string() << "\n";
    }
    return true;
  }

  json load_progress() {
    fs::path path = progress_path();
    if(!fs::exists(path)) {
      json j;
      j["started"] = json::array();
      j["solved"] = json::array();
      j["bookmarks"] = json::array();
      return j;
    }
    std::ifstream fin(path);
    json j;
    fin >> j;
    if(!j.contains("bookmarks")) {
      j["bookmarks"] = json::array();
    }
    return j;
  }

  void save_progress(const json &j) {
    fs::path path = progress_path();
    fs::create_directories(path.parent_path());
    std::ofstream fout(path);
    fout << j.dump(2) << "\n";
  }

  bool in_list(const json &list, const std::string &id) {
    return std::find(list.begin(), list.end(), id) != list.end();
  }

  void print_grouped(const json &list, const std::string &filter) {
    if(list.empty()) {
      std::cout << "None\n";
      return;
    }

    std::vector<std::string> order;
    std::map<std::string, std::vector<std::string>> groups;
    for(const json &item : list) {
      std::string id = item.get<std::string>();
      std::string group = id;
      size_t slash = id.rfind('/');
      if(slash != std::string::npos) {
        group = id.substr(0, slash);
      }
      if(!filter.empty()) {
        bool matches = (group == filter || group.rfind(filter + "/", 0) == 0);
        if(!matches) {
          continue;
        }
      }
      if(groups.find(group) == groups.end()) {
        order.push_back(group);
      }
      groups[group].push_back(id);
    }

    if(order.empty()) {
      std::cout << "None\n";
      return;
    }

    for(const std::string &group : order) {
      std::cout << "== " << group << "\n";
      for(const std::string &id : groups[group]) {
        std::cout << "    " << id << "\n";
      }
    }
  }

  int work_start(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }
    if(!check_problem(id)) {
      return 1;
    }

    json progress = load_progress();
    if(in_list(progress["started"], id.path_str)) {
      std::cout << "Already started: " << id.path_str << "\n";
      return 0;
    }
    if(in_list(progress["solved"], id.path_str)) {
      std::cout << id.path_str << " is already solved; use: lca unsolved " << id.path_str << "\n";
      return 1;
    }

    progress["started"].push_back(id.path_str);
    save_progress(progress);
    std::cout << "Started: " << id.path_str << "\n";
    std::cout << "    " << problem_dir(id).string() << "\n";
    return 0;
  }

  int work_continue(const std::string &filter) {
    json progress = load_progress();
    std::cout << "In progress:\n";
    print_grouped(progress["started"], filter);
    return 0;
  }

  int work_solved(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }

    json progress = load_progress();
    if(in_list(progress["solved"], id.path_str)) {
      std::cout << "Already solved: " << id.path_str << "\n";
      return 0;
    }
    if(!in_list(progress["started"], id.path_str)) {
      std::cout << id.path_str << " is not in progress; use: lca work " << id.path_str << "\n";
      return 1;
    }

    auto &started = progress["started"];
    started.erase(std::remove(started.begin(), started.end(), id.path_str), started.end());
    progress["solved"].push_back(id.path_str);
    auto &bookmarks = progress["bookmarks"];
    auto bm_it = std::find(bookmarks.begin(), bookmarks.end(), id.path_str);
    if(bm_it != bookmarks.end()) {
      bookmarks.erase(bm_it);
      save_progress(progress);
      std::cout << "Solved: " << id.path_str << " (bookmark removed)\n";
    } else {
      save_progress(progress);
      std::cout << "Solved: " << id.path_str << "\n";
    }
    return 0;
  }

  int work_unsolved(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }

    json progress = load_progress();
    if(!in_list(progress["solved"], id.path_str)) {
      std::cout << id.path_str << " is not solved; use: lca solved " << id.path_str << "\n";
      return 1;
    }

    auto &solved = progress["solved"];
    solved.erase(std::remove(solved.begin(), solved.end(), id.path_str), solved.end());
    progress["started"].push_back(id.path_str);
    save_progress(progress);
    std::cout << "Moved back to in progress: " << id.path_str << "\n";
    return 0;
  }

  int work_unwork(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }

    json progress = load_progress();
    bool removed = false;
    auto &started = progress["started"];
    auto old_started_size = started.size();
    started.erase(std::remove(started.begin(), started.end(), id.path_str), started.end());
    removed = (removed || started.size() != old_started_size);
    auto &solved = progress["solved"];
    auto old_solved_size = solved.size();
    solved.erase(std::remove(solved.begin(), solved.end(), id.path_str), solved.end());
    removed = (removed || solved.size() != old_solved_size);
    auto &bookmarks = progress["bookmarks"];
    auto old_bm_size = bookmarks.size();
    bookmarks.erase(std::remove(bookmarks.begin(), bookmarks.end(), id.path_str), bookmarks.end());
    removed = (removed || bookmarks.size() != old_bm_size);

    if(!removed) {
      std::cout << id.path_str << " is not in any list\n";
      return 1;
    }

    save_progress(progress);
    std::cout << "Removed: " << id.path_str << "\n";
    return 0;
  }

  int work_bookmark(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }

    json progress = load_progress();
    if(in_list(progress["bookmarks"], id.path_str)) {
      std::cout << "Already bookmarked: " << id.path_str << "\n";
      return 0;
    }

    progress["bookmarks"].push_back(id.path_str);
    save_progress(progress);
    std::cout << "Bookmarked: " << id.path_str << "\n";
    return 0;
  }

  int work_unbookmark(const std::string &input) {
    ProblemId id;
    std::string error;
    if(!parse_problem_id(input, id, error)) {
      std::cout << error << "\n";
      return 1;
    }

    json progress = load_progress();
    auto &bookmarks = progress["bookmarks"];
    auto it = std::find(bookmarks.begin(), bookmarks.end(), id.path_str);
    if(it == bookmarks.end()) {
      std::cout << id.path_str << " is not bookmarked\n";
      return 1;
    }

    bookmarks.erase(it);
    save_progress(progress);
    std::cout << "Bookmark removed: " << id.path_str << "\n";
    return 0;
  }

  int work_bookmark_list(const std::string &filter) {
    json progress = load_progress();
    std::cout << "Bookmarks:\n";
    print_grouped(progress["bookmarks"], filter);
    return 0;
  }
}

int work_command(int argc, char **argv) {
  if(argc == 0) {
    std::cout << "Work commands\n";
    std::cout << "    lca work <provider>/<contest>/<letter>         start working on a problem\n";
    std::cout << "    lca continue [provider]                        list problems in progress\n";
    std::cout << "    lca solved [provider]                          list solved problems\n";
    std::cout << "    lca solved <provider>/<contest>/<letter>       mark a problem as solved\n";
    std::cout << "    lca unsolved <provider>/<contest>/<letter>     move a solved problem back to in progress\n";
    std::cout << "    lca unwork <provider>/<contest>/<letter>       remove a problem from both lists\n";
    std::cout << "    lca bookmark <provider>/<contest>/<letter>     bookmark a problem\n";
    std::cout << "    lca unbookmark <provider>/<contest>/<letter>   remove a bookmark\n";
    return 0;
  }

  return work_start(argv[0]);
}

int work_continue_command(int argc, char **argv) {
  std::string filter;
  if(argc >= 1) {
    filter = to_lower(argv[0]);
  }
  return work_continue(filter);
}

int work_solved_command(int argc, char **argv) {
  if(argc >= 1 && std::string(argv[0]).find('/') != std::string::npos) {
    return work_solved(argv[0]);
  }
  std::string filter;
  if(argc >= 1) {
    filter = to_lower(argv[0]);
  }
  json progress = load_progress();
  std::cout << "Solved:\n";
  print_grouped(progress["solved"], filter);
  return 0;
}

int work_unsolved_command(int argc, char **argv) {
  if(argc < 1) {
    std::cout << "Usage: lca unsolved <provider>/<contest>/<letter>\n";
    return 1;
  }
  return work_unsolved(argv[0]);
}

int work_unwork_command(int argc, char **argv) {
  if(argc < 1) {
    std::cout << "Usage: lca unwork <provider>/<contest>/<letter>\n";
    return 1;
  }
  return work_unwork(argv[0]);
}

int work_bookmark_command(int argc, char **argv) {
  if(argc < 1) {
    return work_bookmark_list("");
  }
  if(std::string(argv[0]).find('/') != std::string::npos) {
    return work_bookmark(argv[0]);
  }
  return work_bookmark_list(to_lower(argv[0]));
}

int work_unbookmark_command(int argc, char **argv) {
  if(argc < 1) {
    std::cout << "Usage: lca unbookmark <provider>/<contest>/<letter>\n";
    return 1;
  }
  return work_unbookmark(argv[0]);
}