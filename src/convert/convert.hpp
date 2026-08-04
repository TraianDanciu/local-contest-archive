#pragma once

#include <filesystem>
#include <string>

std::string statement_html_to_md(const std::string &html, const std::filesystem::path &statement_files_dir);