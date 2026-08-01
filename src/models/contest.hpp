#pragma once

#include "problem.hpp"
#include <string>
#include <vector>

struct Contest {
  int id;
  std::string name;
  std::string provider;
  int year;
  std::vector<Problem> problems;
};