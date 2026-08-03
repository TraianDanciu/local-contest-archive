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
    contest.year = 0;
    contests.push_back(contest);
  }

  return contests;
}

std::vector<Problem> codeforces_get_problems() {
  std::string body = http_get("https://codeforces.com/api/problemset.problems");
  json j = json::parse(body);

  std::vector<Problem> problems;
  for(auto &problem_json : j["result"]["problems"]) {
    if(!problem_json.contains("contestId")) {
      continue;
    }

    Problem problem;
    problem.contest_id = problem_json["contestId"];
    problem.id = problem_json["index"];
    problem.name = problem_json["name"];
    problems.push_back(problem);
  }

  return problems;
}

void codeforces_download_statement(const Problem &problem, const std::filesystem::path &output_path) {
  std::string url = "https://codeforces.com/contest/" + std::to_string(problem.contest_id) + "/problem/" + problem.id;
  std::string command = "node tools/fetch_statement.js \"" + url + "\" \"" + output_path.string() + "\"";
  int result = std::system(command.c_str());
  if(result != 0) {
    throw std::runtime_error("Failed to download Codeforces statement.");
  }
}