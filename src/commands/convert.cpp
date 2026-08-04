#include "convert.hpp"
#include "../convert/convert.hpp"
#include "../archive/archive.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int convert_command(int argc, char **argv) {
  int only_contest = -1;
  if(argc >= 1) {
    try {
      only_contest = std::stoi(argv[0]);
    } catch(...) {
      std::cout << "Usage: lca convert [contest_id]\n";
      return 1;
    }
  }

  std::filesystem::path provider_path = archive_path() / "codeforces";
  if(!std::filesystem::exists(provider_path)) {
    std::cout << "No codeforces archive found.\n";
    return 0;
  }

  int converted = 0, skipped = 0, pdfs = 0, missing = 0, failed = 0;
  for(const auto &contest_entry : std::filesystem::directory_iterator(provider_path)) {
    if(!contest_entry.is_directory()) {
      continue;
    }

    int contest_id = -1;
    try {
      contest_id = std::stoi(contest_entry.path().filename().string());
    } catch(...) {
      continue;
    }
    if(only_contest != -1 && contest_id != only_contest) {
      continue;
    }

    for(const auto &problem_entry : std::filesystem::directory_iterator(contest_entry.path())) {
      if(!problem_entry.is_directory()) {
        continue;
      }

      std::filesystem::path statement_html = problem_entry.path() / "statement.html";
      std::filesystem::path statement_pdf = problem_entry.path() / "statement.pdf";
      std::filesystem::path statement_md = problem_entry.path() / "statement.md";

      if(!std::filesystem::exists(statement_html)) {
        if(std::filesystem::exists(statement_pdf)) {
          pdfs++;
        } else {
          missing++;
        }
        continue;
      }
      if(std::filesystem::exists(statement_md)) {
        skipped++;
        continue;
      }

      std::ifstream fin(statement_html, std::ios::binary);
      std::stringstream buffer;
      buffer << fin.rdbuf();

      try {
        std::string md = statement_html_to_md(buffer.str(), problem_entry.path() / "statement_files");
        std::ofstream fout(statement_md);
        fout << md;
        converted++;
      } catch(const std::exception &e) {
        std::cerr << "Failed: " << problem_entry.path() << " -> " << e.what() << "\n";
        failed++;
      }

      if((converted + skipped) % 200 == 0) {
        std::cout << "converted " << converted << " (skipped " << skipped << ")\n";
      }
    }
  }

  std::cout << "Done: " << converted << " converted, " << skipped << " already converted, "
            << failed << " failed, " << pdfs << " PDF-only, " << missing << " no statement.\n";
  return failed == 0 ? 0 : 1;
}