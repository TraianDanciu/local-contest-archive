#include "oi.hpp"
#include "../archive/archive.hpp"
#include "../http/http.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
  std::string lowercase(std::string s) {
    for(char &c : s) {
      c = (char)std::tolower((unsigned char)c);
    }
    return s;
  }

  bool ends_with(const std::string &s, const std::string &suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  bool contains(const std::string &s, const std::string &needle) {
    return s.find(needle) != std::string::npos;
  }

  bool starts_with(const std::string &s, const std::string &prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
  }

  bool is_archive(const std::string &name) {
    for(const char *ext : {".tar.xz", ".tar.gz", ".tgz", ".tar", ".zip", ".zst"}) {
      if(ends_with(name, ext)) {
        return true;
      }
    }
    return false;
  }

  bool is_derivative(const std::string &name) {
    for(const char *suffix : {
      "_chocr.html.gz", "_djvu.txt", "_djvu.xml", "_hocr.html", "_hocr_pageindex.json.gz",
      "_hocr_searchtext.txt.gz", "_jp2.zip", "_page_numbers.json", "_scandata.xml",
      "_text.pdf", "_thumb.jpg", "_thumbs.zip", "_xml.zip", "_abbyy.gz"
    }) {
      if(ends_with(name, suffix)) {
        return true;
      }
    }
    if(ends_with(name, ".jp2")) {
      return true;
    }
    if(contains(name, "_thumb") || contains(name, "__ia")) {
      return true;
    }
    return false;
  }

  std::string basename(const std::string &path) {
    std::size_t pos = path.find_last_of('/');
    return pos == std::string::npos ? path : path.substr(pos + 1);
  }

  bool is_test_archive(const std::string &name, const std::string &olympiad) {
    std::string base = lowercase(basename(name));
    return contains(base, "testdata") || contains(base, "tests") ||
           contains(base, "inputs") || ends_with(base, "-data.tar.xz") ||
           contains(name, "/testdata/") || contains(name, "/tests/") || contains(name, "/secret/");
  }

  bool is_redundant_archive(const std::string &name, const std::string &olympiad) {
    if(olympiad != "apio") {
      return false;
    }
    std::string base = lowercase(basename(name));
    return contains(base, "archive") || contains(base, "full");
  }

  bool is_practice_archive(const std::string &name) {
    std::string base = lowercase(basename(name));
    return contains(base, "practice") || contains(base, "practise");
  }

  bool is_editorial_video(const std::string &name) {
    return contains(lowercase(name), "editorial-video");
  }

  std::string url_encode(const std::string &s) {
    std::string out;
    char buf[4];
    for(unsigned char c : s) {
      if(std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == '~') {
        out += (char)c;
      } else {
        std::snprintf(buf, sizeof(buf), "%%%02X", (unsigned int)c);
        out += buf;
      }
    }
    return out;
  }

  std::string download_url(const std::string &item, const std::string &path) {
    return "https://archive.org/download/" + item + "/" + url_encode(path);
  }

  json fetch_metadata(const std::string &item) {
    std::string body = http_get("https://archive.org/metadata/" + item);
    json j = json::parse(body);
    if(!j.contains("files") || !j["files"].is_array()) {
      throw std::runtime_error("Archive.org metadata has no files list for " + item);
    }
    return j;
  }

  std::unique_ptr<std::cmatch> match_year(const OiOlympiad &olympiad, const std::string &path) {
    std::regex re(olympiad.year_regex);
    std::cmatch match;
    if(std::regex_search(path.c_str(), match, re)) {
      return std::make_unique<std::cmatch>(match);
    }
    return nullptr;
  }

  std::string normalize_year(std::string year) {
    for(char &c : year) {
      if(c == '-') {
        c = '_';
      }
    }
    return year;
  }

  int download_file(const std::string &url, const std::filesystem::path &dest) {
    std::string tmp = dest.string() + ".part";
    std::string command = "curl -fsSL --retry 3 -o \"" + tmp + "\" \"" + url + "\"";
    int status = std::system(command.c_str());
    if(status != 0 || !std::filesystem::exists(tmp)) {
      std::filesystem::remove(tmp);
      return 1;
    }
    std::filesystem::rename(tmp, dest);
    return 0;
  }

  bool extract_archive(const std::filesystem::path &file, const std::filesystem::path &dest) {
    std::string command;
    if(file.extension() == ".zip") {
      command = "unzip -o -q \"" + file.string() + "\" -d \"" + dest.string() + "\"";
    } else {
      command = "tar -xf \"" + file.string() + "\" -C \"" + dest.string() + "\"";
    }
    return std::system(command.c_str()) == 0;
  }

  long long parse_size(const json &file) {
    try {
      if(file.contains("size")) {
        return std::stoll(file["size"].get<std::string>());
      }
    } catch(...) {}
    return 0;
  }

  void cleanup_tests(const fs::path &dir) {
    if(!fs::exists(dir)) {
      return;
    }
    std::vector<fs::path> to_remove;
    for(fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied), end; it != end; ++it) {
      const fs::path &path = it->path();
      if(it->is_directory()) {
        std::string name = lowercase(path.filename().string());
        if(contains(name, "testdata") || contains(name, "test_data") ||
           contains(name, "tests") || contains(name, "secret") ||
           contains(name, "practice") || contains(name, "input") ||
           contains(name, "output") || contains(name, "gen")) {
          to_remove.push_back(path);
        }
      } else if(it->is_regular_file()) {
        std::string ext = lowercase(path.extension().string());
        std::string stem = lowercase(path.stem().string());
        std::string filename = lowercase(path.filename().string());
        if(ext == ".in" || ext == ".out" || ext == ".ok" || ext == ".ans" ||
           ext == ".mkv" || ext == ".mp4") {
          to_remove.push_back(path);
          continue;
        }
        if(ends_with(filename, "-full.zip") || ends_with(filename, "_full.zip") ||
           contains(filename, "testcase") || contains(filename, "test_data")) {
          to_remove.push_back(path);
          continue;
        }
        bool numeric_ext = ext.size() > 1 &&
          std::all_of(ext.begin() + 1, ext.end(), [](unsigned char c) { return std::isdigit(c); });
        if(numeric_ext && (stem == "in" || stem == "out" || stem == "input" || stem == "output")) {
          to_remove.push_back(path);
        }
      }
    }
    for(const fs::path &path : to_remove) {
      std::error_code ec;
      if(fs::is_directory(path)) {
        fs::remove_all(path, ec);
      } else {
        fs::remove(path, ec);
      }
    }
  }

  std::string group_name(std::string name) {
    std::string result = lowercase(name);
    bool stripped = true;
    while(stripped) {
      stripped = false;
      for(const std::string &suffix : {"-ex", "_ex", "-isc", "_isc", "-en", "_en"}) {
        if(ends_with(result, suffix)) {
          result = result.substr(0, result.size() - suffix.size());
          stripped = true;
          break;
        }
      }
    }
    if(result.size() > 5 && std::isdigit((unsigned char)result[0]) &&
       std::isdigit((unsigned char)result[1]) && std::isdigit((unsigned char)result[2]) &&
       std::isdigit((unsigned char)result[3]) && result[4] == '_') {
      result = result.substr(5);
    }
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return c == '_' || c == '-'; }), result.end());
    return result;
  }

  bool excluded_stem(const std::string &stem) {
    static const std::vector<std::string> keywords = {
      "task", "day", "contest", "olympiad", "notice", "solution", "editorial",
      "practice", "test", "review", "result", "scoreboard", "rank", "sample",
      "overview", "translation", "statement", "misc", "notes", "archive",
      "grader", "checker", "validator", "contestant", "manager", "submission", "tool",
    };
    if(stem == "ov" || stem == "en" || stem == "ti") {
      return true;
    }
    if(ends_with(stem, "-ov") || ends_with(stem, "-en") || ends_with(stem, "-ti")) {
      return true;
    }
    for(const std::string &kw : keywords) {
      if(contains(stem, kw)) {
        return true;
      }
    }
    return false;
  }

  int problem_number(const std::string &name) {
    std::regex re("problem([0-9]+)");
    std::smatch match;
    if(std::regex_search(name, match, re)) {
      return std::stoi(match[1].str());
    }
    return -1;
  }

  void normalize_year(const OiOlympiad &olympiad, const fs::path &year_dir) {
    static const std::set<std::string> skipped_olympiads = {"coci", "joi", "joioc", "joisc"};
    if(skipped_olympiads.count(olympiad.name)) {
      return;
    }
    if(!fs::exists(year_dir)) {
      return;
    }

    bool already_normalized = false;
    for(const auto &entry : fs::directory_iterator(year_dir)) {
      if(entry.is_directory() && entry.path().filename().string().size() == 1 &&
         std::isalpha((unsigned char)entry.path().filename().string()[0])) {
        already_normalized = true;
      }
    }
    if(already_normalized) {
      return;
    }

    std::map<std::string, std::vector<fs::path>> groups;
    for(const auto &entry : fs::directory_iterator(year_dir)) {
      std::string name = entry.path().filename().string();
      if(entry.is_directory()) {
        std::string lower = lowercase(name);
        static const std::set<std::string> excluded_dirs = {
          "editorial", "editorials", "grader", "graders", "solutions", "practice",
          "testdata", "tests", "secret", "day1", "day2", "day-1", "day-2",
          "notes", "misc", "translations", "translation", "samples",
        };
        if(excluded_dirs.count(lower) || contains(lower, "translation") ||
           contains(lower, "solution") || contains(lower, "validator") ||
           excluded_stem(lower) || starts_with(lower, olympiad.name + "-")) {
          continue;
        }
        groups[group_name(name)].push_back(entry.path());
      } else {
        if(entry.path().extension() != ".pdf" && entry.path().extension() != ".PDF") {
          continue;
        }
        std::string stem = lowercase(entry.path().stem().string());
        if(excluded_stem(stem)) {
          continue;
        }
        groups[group_name(stem)].push_back(entry.path());
      }
    }

    if(groups.size() < 2 || groups.size() > 8) {
      return;
    }

    std::vector<std::pair<std::string, int>> order;
    for(const auto &group : groups) {
      order.push_back({group.first, problem_number(group.first)});
    }
    std::sort(order.begin(), order.end(), [](const auto &a, const auto &b) {
      if(a.second != -1 && b.second != -1) {
        return a.second < b.second;
      }
      if(a.second != -1) {
        return true;
      }
      if(b.second != -1) {
        return false;
      }
      return a.first < b.first;
    });

    for(std::size_t i = 0; i < order.size(); i++) {
      std::string letter(1, (char)('A' + i));
      fs::path letter_dir = year_dir / letter;
      fs::create_directories(letter_dir);
      for(const fs::path &item : groups[order[i].first]) {
        std::error_code ec;
        fs::rename(item, letter_dir / item.filename(), ec);
      }
    }
  }
}

const std::vector<OiOlympiad> &oi_olympiads() {
  static const std::vector<OiOlympiad> olympiads = {
    {"apio",  "Asia-Pacific Informatics Olympiad",        "oi-ted-apio-archive", "^APIO/([0-9]{4})/"},
    {"baltoi","Baltic Olympiad in Informatics",           "oi-ted-BOI-archive",  "^BOI/([0-9]{4})/"},
    {"ceoi",  "Central European Olympiad in Informatics", "oi-ted-ceoi-archive", "^CEOI/([0-9]{4})/"},
    {"coci",  "Croatian Open Competition in Informatics", "oi-ted-coci-archive", "^COCI-COI/([0-9]{4}_[0-9]{4})/"},
    {"egoi",  "European Girls' Olympiad in Informatics",  "oi-ted-egoi-archive", "^EGOI/([0-9]{4})/"},
    {"ejoi",  "European Junior Olympiad in Informatics",  "oi-ted-ejoi-archive", "^EJOI/([0-9]{4})/"},
    {"ioi",   "International Olympiad in Informatics",    "oi-ted-ioi-archive",  "^IOI/ioi-([0-9]{4})/"},
    {"joi",   "JOI Final (English)",                      "oi-ted-joi-archive",  "^JOI\\.ENG/JOI\\.FINAL\\.ENG[^/]*/JOI Final ([0-9]{4})-ho/"},
    {"joioc", "JOI Open Contest",                         "oi-ted-joi-archive",  "^JOI\\.ENG/JOIOC\\.EN\\.JP/open-([0-9]{4})/"},
    {"joisc", "JOI Spring Camp (English)",                "oi-ted-joi-archive",  "^JOI\\.ENG/JOISC\\.SpringCamp\\.ENG[^/]*/([0-9]{4})-sp-tasks/"},
  };
  return olympiads;
}

const OiOlympiad *oi_find_olympiad(const std::string &name) {
  std::string lower = lowercase(name);
  for(const OiOlympiad &olympiad : oi_olympiads()) {
    if(olympiad.name == lower) {
      return &olympiad;
    }
  }
  return nullptr;
}

std::vector<OiYear> oi_list_years(const OiOlympiad &olympiad) {
  json metadata = fetch_metadata(olympiad.item);
  std::map<std::string, OiYear> years;
  for(const json &file : metadata["files"]) {
    std::string name = file.value("name", "");
    if(is_derivative(name) || is_test_archive(name, olympiad.name) ||
       is_redundant_archive(name, olympiad.name) ||
       is_practice_archive(name) || is_editorial_video(name)) {
      continue;
    }
    auto match = match_year(olympiad, name);
    if(!match) {
      continue;
    }
    OiYear &year = years[(*match)[1].str()];
    year.year = (*match)[1].str();
    year.size += parse_size(file);
  }
  std::vector<OiYear> result;
  for(auto &entry : years) {
    result.push_back(entry.second);
  }
  return result;
}

int oi_update(const OiOlympiad &olympiad, const std::vector<std::string> &years) {
  std::vector<OiYear> available = oi_list_years(olympiad);

  std::vector<OiYear> selected;
  if(years.empty()) {
    selected = available;
  } else {
    for(const std::string &wanted : years) {
      std::string normalized = normalize_year(wanted);
      for(const OiYear &year : available) {
        if(year.year == normalized) {
          selected.push_back(year);
          break;
        }
      }
    }
  }

  std::cout << "Updating " << olympiad.name << ": " << selected.size() << " years\n";

  std::filesystem::path provider_path = archive_path() / olympiad.name;
  std::filesystem::create_directories(provider_path);

  json metadata = fetch_metadata(olympiad.item);
  std::vector<std::string> files;
  for(const json &file : metadata["files"]) {
    std::string name = file.value("name", "");
    if(!is_derivative(name) && !is_test_archive(name, olympiad.name) &&
       !is_redundant_archive(name, olympiad.name) &&
       !is_practice_archive(name) && !is_editorial_video(name)) {
      files.push_back(name);
    }
  }

  int total_extracted = 0, total_copied = 0, total_skipped = 0, total_failed = 0;
  for(const OiYear &year : selected) {
    std::filesystem::path target = provider_path / year.year;
    std::filesystem::create_directories(target);

    std::set<std::string> done;
    std::filesystem::path manifest_path = target / ".lca-manifest.json";
    if(std::filesystem::exists(manifest_path)) {
      std::ifstream fin(manifest_path);
      json j;
      try {
        fin >> j;
        for(const json &entry : j) {
          done.insert(entry.get<std::string>());
        }
      } catch(...) {}
    }

    std::vector<std::string> selected_files;
    for(const std::string &source : files) {
      auto match = match_year(olympiad, source);
      if(match && (*match)[1].str() == year.year) {
        selected_files.push_back(source);
      }
    }

    json contest_json;
    contest_json["id"] = year.year;
    contest_json["name"] = olympiad.display + " " + year.year;
    int numeric_year = 0;
    try {
      numeric_year = std::stoi(year.year);
    } catch(...) {}
    contest_json["year"] = numeric_year;
    std::ofstream contest_out(target / "contest.json");
    contest_out << contest_json.dump(4);

    if(selected_files.empty()) {
      std::cout << olympiad.name << "/" << year.year << ": no files found\n";
      continue;
    }

    int extracted = 0, copied = 0, skipped = 0, failed = 0;
    for(const std::string &source : selected_files) {
      if(done.count(source)) {
        skipped++;
        continue;
      }

      auto match = match_year(olympiad, source);
      std::filesystem::path local = target / match->suffix().str();
      std::error_code ec;
      std::filesystem::create_directories(local.parent_path(), ec);

      std::string url = download_url(olympiad.item, source);
      if(is_archive(source)) {
        std::filesystem::path extract_dir = local.parent_path();
        std::string base = lowercase(basename(source));
        if(contains(base, "solution")) {
          extract_dir = target / "solutions";
        }
        std::filesystem::create_directories(extract_dir, ec);

        int status = download_file(url, local);
        if(status != 0 || !std::filesystem::exists(local)) {
          std::cerr << "  failed to download " << source << "\n";
          failed++;
          continue;
        }
        if(extract_archive(local, extract_dir)) {
          std::filesystem::remove(local, ec);
          extracted++;
        } else {
          std::cerr << "  failed to extract " << source << "\n";
          failed++;
          continue;
        }
      } else {
        int status = download_file(url, local);
        if(status != 0 || !std::filesystem::exists(local)) {
          std::cerr << "  failed to download " << source << "\n";
          failed++;
          continue;
        }
        copied++;
      }

      done.insert(source);
      cleanup_tests(target);

      json manifest = json::array();
      for(const std::string &entry : done) {
        manifest.push_back(entry);
      }
      std::ofstream fout(manifest_path);
      fout << manifest.dump(2);
    }

    cleanup_tests(target);
    normalize_year(olympiad, target);

    std::cout << olympiad.name << "/" << year.year << ": " << extracted << " extracted, "
              << copied << " copied, " << skipped << " skipped, " << failed << " failed\n";
    total_extracted += extracted;
    total_copied += copied;
    total_skipped += skipped;
    total_failed += failed;
  }

  std::cout << olympiad.name << ": " << total_extracted << " extracted, "
            << total_copied << " copied, " << total_skipped << " skipped, "
            << total_failed << " failed\n";
  return total_failed == 0 ? 0 : 1;
}

int oi_status() {
  long long total_size = 0;
  int total_years = 0;
  for(const OiOlympiad &olympiad : oi_olympiads()) {
    std::filesystem::path provider_path = archive_path() / olympiad.name;
    if(!std::filesystem::exists(provider_path)) {
      continue;
    }
    std::cout << "== " << olympiad.name << "\n";
    for(const auto &year_entry : std::filesystem::directory_iterator(provider_path)) {
      if(!year_entry.is_directory()) {
        continue;
      }
      long long size = 0;
      int files = 0;
      for(const auto &file_entry : std::filesystem::recursive_directory_iterator(year_entry.path())) {
        if(file_entry.is_regular_file() && file_entry.path().filename() != ".lca-manifest.json") {
          size += file_entry.file_size();
          files++;
        }
      }
      std::cout << "    " << year_entry.path().filename().string()
                << "  " << files << " files  " << (double)size / (1024.0 * 1024.0) << " MB\n";
      total_size += size;
      total_years++;
    }
  }
  std::cout << "Total: " << total_years << " years, " << (double)total_size / (1024.0 * 1024.0) << " MB\n";
  return 0;
}
