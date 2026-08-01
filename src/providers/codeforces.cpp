#include "codeforces.hpp"
#include "../http/http.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<Contest> codeforces_get_contests() {
  std::string body = http_get("https://codeforces.com/api/contest.list");
  json j = json::parse(body);
  
  std::vector<Contest> contests;
  for(auto &contest_json : j["result"]) {
    Contest contest;
    contest.id = contest_json["id"];
    contest.name = contest_json["name"];
    contest.provider = "codeforces";
    contest.year = 0;
    contests.push_back(contest);
  }

  return contests;
}