#pragma once

#include "../models/contest.hpp"
#include "../models/problem.hpp"
#include <vector>

std::vector<Contest> codeforces_get_contests();

std::vector<Problem> codeforces_get_problems();