#include "codeforces.hpp"
#include "../http/http.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

std::string codeforces_problem_url(const Problem &problem) {
  return "https://codeforces.com/contest/" + std::to_string(problem.contest_id) + "/problem/" + problem.id;
}

std::vector<Contest> codeforces_get_contests() {
  std::string body = http_get("https://codeforces.com/api/contest.list");
  json j = json::parse(body);
  if(j["status"] != "OK") {
    throw std::runtime_error("Codeforces API Error: " + j.value("comment", "unknown"));
  }
  
  std::vector<Contest> contests;
  for(auto &contest_json : j["result"]) {
    if(contest_json["phase"] != "FINISHED") {
      continue;
    }
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
  if(j["status"] != "OK") {
    throw std::runtime_error("Codeforces API Error: " + j.value("comment", "unknown"));
  }

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

int codeforces_download_statements(const std::filesystem::path &job_file) {
  std::string script = std::string(LCA_TOOLS_DIR) + "/fetch_statement.js";
  std::string command = "node --max-old-space-size=4096 \"" + script + "\" \"" + job_file.string() + "\"";
  return std::system(command.c_str());
}