#pragma once

#include <string>
#include <vector>

struct AtcoderTask {
  std::string id;
  std::string letter;
  std::string title;
  std::string time_limit;
  std::string memory_limit;
};

struct AtcoderContest {
  std::string id;
  std::string name;
  std::vector<AtcoderTask> tasks;
};

std::vector<std::string> atcoder_list_contests();

AtcoderContest atcoder_fetch_contest(const std::string &contest_id);

std::string atcoder_build_statement_html(const std::string &contest_id, const AtcoderTask &task);