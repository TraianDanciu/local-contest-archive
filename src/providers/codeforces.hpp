#pragma once

#include "../models/contest.hpp"
#include "../models/problem.hpp"
#include <vector>
#include <string>
#include <filesystem>

std::string codeforces_problem_url(const Problem &problem);

std::vector<Contest> codeforces_get_contests();

std::vector<Problem> codeforces_get_problems();

int codeforces_download_statements(const std::filesystem::path &job_file);