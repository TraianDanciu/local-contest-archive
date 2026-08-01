#include "archive.hpp"
#include <cstdlib>

std::filesystem::path archive_path() {
  char *home = std::getenv("HOME");

  if(home == nullptr) {
    throw std::runtime_error("HOME environment variable not found.");
  }

  return std::filesystem::path(home) / ".local" / "share" / "lca" / "archive";
}

void archive_save_codeforces(const std::vector<Contest> &contests) {
  std::filesystem::path path = archive_path() / "codeforces";
  std::filesystem::create_directories(path);
}

std::vector<Contest> archive_load_codeforces() {
  return {};
}