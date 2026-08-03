#pragma once

#include "../models/contest.hpp"
#include <filesystem>
#include <vector>

std::filesystem::path archive_path();
std::filesystem::path archive_problem_path(const std::string &provider, const Problem &problem);
std::filesystem::path archive_statement_path(const std::string &provider, const Problem &problem);

void archive_save_codeforces(const std::vector<Contest> &contests, const std::vector<Problem> &problems);

std::vector<Contest> archive_load_codeforces();