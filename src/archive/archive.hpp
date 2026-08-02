#pragma once

#include "../models/contest.hpp"
#include <filesystem>
#include <vector>

std::filesystem::path archive_path();

void archive_save_codeforces(const std::vector<Contest> &contests, const std::vector<Problem> &problems);

std::vector<Contest> archive_load_codeforces();

void archive_save_codeforces_statement(const Problem &problem, const std::string &html);