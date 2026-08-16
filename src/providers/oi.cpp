#include "oi.hpp"
#include "../archive/archive.hpp"
#include "../http/http.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
  const std::string kDbIndexDefault =
      "https://raw.githubusercontent.com/TraianDanciu/lca-oi-db/main/index.json";

  std::string db_index_url() {
    const char *override_url = std::getenv("LCA_OI_DB_INDEX");
    if(override_url != nullptr && *override_url != '\0') {
      return override_url;
    }
    return kDbIndexDefault;
  }

  std::string lowercase(std::string s) {
    for(char &c : s) {
      c = (char)std::tolower((unsigned char)c);
    }
    return s;
  }

  bool year_installed(const fs::path &provider_path, const std::string &year) {
    fs::path dir = provider_path / year;
    if(!fs::exists(dir)) {
      return false;
    }
    for(fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied), end; it != end; ++it) {
      if(it->is_regular_file()) {
        return true;
      }
    }
    return false;
  }

  int download_file(const std::string &url, const fs::path &dest) {
    fs::path tmp = dest.string() + ".part";
    try {
      http_get_file(url, tmp.string());
    } catch(const std::exception &e) {
      std::cerr << "  download failed: " << e.what() << "\n";
      fs::remove(tmp);
      return 1;
    }
    if(!fs::exists(tmp)) {
      return 1;
    }
    fs::rename(tmp, dest);
    return 0;
  }

  bool extract_tarball(const fs::path &file, const fs::path &dest) {
    fs::create_directories(dest);
    std::string command = "tar -xzf \"" + file.string() + "\" -C \"" + dest.string() + "\"";
    return std::system(command.c_str()) == 0;
  }
}

const std::vector<OiOlympiad> &oi_olympiads() {
  static const std::vector<OiOlympiad> olympiads = {
    {"apio",   "Asia-Pacific Olympiad in Informatics"},
    {"baltoi", "Baltic Olympiad in Informatics"},
    {"ceoi",   "Central European Olympiad in Informatics"},
    {"egoi",   "European Girls' Olympiad in Informatics"},
    {"ejoi",   "European Junior Olympiad in Informatics"},
    {"ioi",    "International Olympiad in Informatics"},
    {"joi",    "JOI Final (English)"},
    {"joioc",  "JOI Open Contest"},
    {"joisc",  "JOI Spring Camp (English)"},
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
  std::string body = http_get(db_index_url());
  json index = json::parse(body);

  std::vector<OiYear> years;
  if(!index.contains(olympiad.name)) {
    return years;
  }
  const json &entry = index[olympiad.name];
  if(!entry.contains("years") || !entry["years"].is_object()) {
    return years;
  }
  for(auto it = entry["years"].begin(); it != entry["years"].end(); ++it) {
    OiYear year;
    year.year = it.key();
    year.size = it.value().value("size", 0LL);
    year.url = it.value().value("url", "");
    years.push_back(year);
  }
  std::sort(years.begin(), years.end(), [](const OiYear &a, const OiYear &b) {
    return a.year < b.year;
  });
  return years;
}

int oi_update(const OiOlympiad &olympiad, const std::vector<std::string> &years) {
  std::vector<OiYear> available = oi_list_years(olympiad);

  std::vector<OiYear> selected;
  if(years.empty()) {
    selected = available;
  } else {
    for(const std::string &wanted : years) {
      bool found = false;
      for(const OiYear &year : available) {
        if(year.year == wanted) {
          selected.push_back(year);
          found = true;
          break;
        }
      }
      if(!found) {
        std::cout << olympiad.name << "/" << wanted << ": not found in database\n";
      }
    }
  }

  if(selected.empty()) {
    std::cout << olympiad.name << ": no years selected\n";
    return 1;
  }

  fs::path provider_path = archive_path() / olympiad.name;
  fs::create_directories(provider_path);

  int total_installed = 0, total_skipped = 0, total_failed = 0;
  for(const OiYear &year : selected) {
    fs::path target = provider_path / year.year;
    if(year_installed(provider_path, year.year)) {
      std::cout << olympiad.name << "/" << year.year << ": already present\n";
      total_skipped++;
      continue;
    }

    fs::path tarball = provider_path / (year.year + ".tar.gz");
    std::cout << olympiad.name << "/" << year.year << ": downloading " << year.url << "\n";
    if(download_file(year.url, tarball) != 0) {
      total_failed++;
      continue;
    }

    if(extract_tarball(tarball, provider_path)) {
      std::error_code ec;
      fs::remove(tarball, ec);
      total_installed++;
    } else {
      std::cerr << "  failed to extract " << tarball.string() << "\n";
      std::error_code ec;
      fs::remove(tarball, ec);
      total_failed++;
    }
  }

  std::cout << olympiad.name << ": " << total_installed << " installed, "
            << total_skipped << " skipped, " << total_failed << " failed\n";
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
        if(file_entry.is_regular_file()) {
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
