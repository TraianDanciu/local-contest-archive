#pragma once

#include <filesystem>
#include <string>

int convert_provider(const std::filesystem::path &provider_path, const std::string &only_contest);
int convert_command();