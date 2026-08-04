#include "convert.hpp"
#include "../convert/convert.hpp"
#include "../archive/archive.hpp"
#include <exception>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int convert_provider(const std::filesystem::path &provider_path, const std::string &only_contest) {
  if(!std::filesystem::exists(provider_path)) {
    std::cout << "No " << provider_path.filename().string() << " archive found.\n";
    return 0;
  }

  int converted = 0, skipped = 0, pdfs = 0, missing = 0, failed = 0;
  for(const auto &contest_entry : std::filesystem::directory_iterator(provider_path)) {
    if(!contest_entry.is_directory()) {
      continue;
    }

    std::string contest_id = contest_entry.path().filename().string();
    if(!only_contest.empty() && contest_id != only_contest) {
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

int convert_command() {
  std::filesystem::path root = archive_path();
  if(!std::filesystem::exists(root)) {
    std::cout << "No archive found\n";
    return 0;
  }

  int status = 0;
  for(const auto &provider_entry : std::filesystem::directory_iterator(root)) {
    if(!provider_entry.is_directory()) {
      continue;
    }
    std::cout << "== " << provider_entry.path().filename().string() << "\n";
    status |= convert_provider(provider_entry.path(), "");
  }
  return status;
}