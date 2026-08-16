#pragma once

#include <string>
#include <vector>

struct OiOlympiad {
  std::string name;
  std::string display;
};

struct OiYear {
  std::string year;
  long long size;
  std::string url;
};

const std::vector<OiOlympiad> &oi_olympiads();
const OiOlympiad *oi_find_olympiad(const std::string &name);

std::vector<OiYear> oi_list_years(const OiOlympiad &olympiad);
int oi_update(const OiOlympiad &olympiad, const std::vector<std::string> &years);
int oi_status();
