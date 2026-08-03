#pragma once

#include "../models/contest.hpp"
#include "../models/problem.hpp"
#include <vector>
#include <string>

std::vector<Contest> codeforces_get_contests();

std::vector<Problem> codeforces_get_problems();

void codeforces_download_statement(const Problem &problem, const std::filesystem::path &output_path);