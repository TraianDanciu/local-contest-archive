#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct OiOlympiad {
  std::string name;
  std::string display;
  std::string item;
  std::string year_regex;
};

struct OiYear {
  std::string year;
  long long size;
};

const std::vector<OiOlympiad> &oi_olympiads();
const OiOlympiad *oi_find_olympiad(const std::string &name);

std::vector<OiYear> oi_list_years(const OiOlympiad &olympiad);
int oi_update(const OiOlympiad &olympiad, const std::vector<std::string> &years);
int oi_status();
