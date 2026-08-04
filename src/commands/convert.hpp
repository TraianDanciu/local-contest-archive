#pragma once

#include <filesystem>
#include <string>
#include <vector>

int convert_provider(const std::filesystem::path &provider_path, const std::vector<std::string> &only_contests);
int convert_command();