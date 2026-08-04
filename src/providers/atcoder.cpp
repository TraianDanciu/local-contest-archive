#include "atcoder.hpp"
#include "../http/http.hpp"
#include <cctype>
#include <string>
#include <thread>

namespace {
  std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while(a < b && std::isspace((unsigned char)s[a])) {
      a++;
    }
    while(b > a && std::isspace((unsigned char)s[b - 1])) {
      b--;
    }
    return s.substr(a, b - a);
  }

  bool contest_id_of_type(const std::string &id, const std::string &prefix) {
    if(id.size() <= prefix.size() || id.compare(0, prefix.size(), prefix) != 0) {
      return false;
    }
    for(size_t i = prefix.size(); i < id.size(); i++) {
      if(!std::isdigit((unsigned char)id[i])) {
        return false;
      }
    }
    return true;
  }

  void parse_links(const std::string &s, std::vector<std::pair<std::string, std::string>> &out) {
    size_t p = 0;
    while(true) {
      size_t a = s.find("<a ", p);
      if(a == std::string::npos) {
        break;
      }
      size_t href_pos = s.find("href=\"", a);
      if(href_pos == std::string::npos || href_pos > a + 300) {
        p = a + 3;
        continue;
      }
      size_t href_end = s.find('"', href_pos + 6);
      if(href_end == std::string::npos) {
        break;
      }
      size_t gt = s.find('>', href_end);
      size_t lt = s.find('<', gt);
      if(gt == std::string::npos || lt == std::string::npos) {
        break;
      }
      out.push_back({s.substr(href_pos + 6, href_end - href_pos - 6), s.substr(gt + 1, lt - gt - 1)});
      p = lt + 1;
    }
  }

  std::vector<std::string> text_right_cells(const std::string &s) {
    std::vector<std::string> v;
    size_t p = 0;
    while(true) {
      size_t t = s.find("<td class=\"text-right\">", p);
      if(t == std::string::npos) {
        break;
      }
      size_t start = t + std::string("<td class=\"text-right\">").size();
      size_t e = s.find("</td>", start);
      if(e == std::string::npos) {
        break;
      }
      v.push_back(s.substr(start, e - start));
      p = e + 5;
    }
    return v;
  }

  std::string meta_og_title(const std::string &html) {
    size_t p = html.find("og:title");
    if(p == std::string::npos) {
      return "";
    }
    size_t c = html.find("content=\"", p);
    if(c == std::string::npos) {
      return "";
    }
    size_t e = html.find('"', c + 9);
    if(e == std::string::npos) {
      return "";
    }
    std::string title = html.substr(c + 9, e - c - 9);
    if(title.rfind("Tasks - ", 0) == 0) {
      title = title.substr(8);
    }
    return title;
  }

  std::string extract_span_content(const std::string &html, const std::string &class_name) {
    std::string needle = "<span class=\"" + class_name + "\"";
    size_t start = html.find(needle);
    if(start == std::string::npos) {
      return "";
    }
    size_t content_start = html.find('>', start);
    if(content_start == std::string::npos) {
      return "";
    }
    content_start++;

    size_t depth = 1, p = content_start;
    while(p < html.size() && depth > 0) {
      size_t next_open = html.find("<span", p);
      size_t next_close = html.find("</span>", p);
      if(next_close == std::string::npos) {
        break;
      }
      if(next_open != std::string::npos && next_open < next_close) {
        depth++;
        p = next_open + 5;
      } else {
        depth--;
        p = next_close + 7;
      }
    }
    if(p >= html.size()) {
      return "";
    }
    size_t content_end = p - 7;
    if(content_end <= content_start) {
      return "";
    }
    return html.substr(content_start, content_end - content_start);
  }

  std::string extract_task_statement(const std::string &html) {
    std::string needle = "<div id=\"task-statement\"";
    size_t start = html.find(needle);
    if(start == std::string::npos) {
      return "";
    }
    size_t content_start = html.find('>', start);
    if(content_start == std::string::npos) {
      return "";
    }
    content_start++;

    size_t depth = 1;
    size_t p = content_start;
    while(p < html.size() && depth > 0) {
      size_t next_open = html.find("<div", p);
      size_t next_close = html.find("</div>", p);
      if(next_close == std::string::npos) {
        break;
      }
      if(next_open != std::string::npos && next_open < next_close) {
        depth++;
        p = next_open + 4;
      } else {
        depth--;
        p = next_close + 6;
      }
    }
    if(p >= html.size()) {
      return "";
    }
    size_t content_end = p - 6;
    if(content_end <= content_start) {
      return "";
    }
    return html.substr(content_start, content_end - content_start);
  }

  void strip_hr(std::string &s) {
    std::string out;
    out.reserve(s.size());
    size_t p = 0;
    while(true) {
      size_t h = s.find("<hr", p);
      if(h == std::string::npos) {
        out += s.substr(p);
        break;
      }
      out += s.substr(p, h - p);
      size_t e = s.find('>', h);
      if(e == std::string::npos) {
        break;
      }
      p = e + 1;
    }
    s = std::move(out);
  }

  void strip_cr(std::string &s) {
    std::string out;
    out.reserve(s.size());
    for(char c : s) {
      if(c != '\r') {
        out += c;
      }
    }
    s = std::move(out);
  }
}

std::vector<std::string> atcoder_list_contests() {
  std::vector<std::string> ids;
  for(const std::string &type : {"abc", "arc", "agc"}) {
    for(int page = 1; page <= 100; page++) {
      std::string url = "https://atcoder.jp/contests/archive?lang=en&page="
                      + std::to_string(page) + "&ratedType=" +
                      (type == "abc" ? "1" : (type == "arc" ? "2" : "3"));
      std::string html;
      try {
        html = http_get(url);
      } catch(...) {
        break;
      }

      std::vector<std::pair<std::string, std::string>> links;
      parse_links(html, links);

      bool any = false;
      for(const auto &link : links) {
        if(link.first.rfind("/contests/", 0) != 0) {
          continue;
        }
        std::string id = link.first.substr(10);
        if(contest_id_of_type(id, type)) {
          ids.push_back(id);
          any = true;
        }
      }
      if(!any) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    int early_last = (type == "abc" ? 41 : (type == "arc" ? 57 : 0));
    for(int n = early_last; n >= 1; n--) {
      ids.push_back(type + (n < 10 ? "00" : "0") + std::to_string(n));
    }
  }
  return ids;
}

AtcoderContest atcoder_fetch_contest(const std::string &contest_id) {
  std::string html = http_get("https://atcoder.jp/contests/" + contest_id + "/tasks?lang=en");
  strip_cr(html);

  AtcoderContest contest;
  contest.id = contest_id;
  contest.name = meta_og_title(html);
  if(contest.name.empty()) {
    contest.name = contest_id;
  }

  size_t p = 0;
  while(true) {
    size_t tr_start = html.find("<tr", p);
    if(tr_start == std::string::npos) {
      break;
    }
    size_t tr_end = html.find("</tr>", tr_start);
    if(tr_end == std::string::npos) {
      break;
    }
    std::string tr = html.substr(tr_start, tr_end - tr_start + 5);
    p = tr_end + 5;

    std::vector<std::pair<std::string, std::string>> links;
    parse_links(tr, links);

    std::string task_id, letter, title;
    int task_links = 0;
    for(const auto &link : links) {
      if(link.first.find("/tasks/") == std::string::npos) {
        continue;
      }
      task_links++;
      if(task_links == 1) {
        task_id = link.first.substr(link.first.find("/tasks") + 7);
        letter = link.second;
      } else if(task_links == 2) {
        title = link.second;
        break;
      }
    }
    if(task_id.empty() || title.empty()) {
      continue;
    }

    std::vector<std::string> cells = text_right_cells(tr);
    std::string time_limit = cells.size() > 0 ? trim(cells[0]) : "";
    std::string memory_limit = cells.size() > 1 ? trim(cells[1]) : "";

    AtcoderTask task;
    task.id = task_id;
    task.letter = trim(letter);
    task.title = trim(title);
    task.time_limit = time_limit;
    task.memory_limit = memory_limit;
    contest.tasks.push_back(task);
  }

  return contest;
}

std::string atcoder_build_statement_html(const std::string &contest_id, const AtcoderTask &task) {
  std::string url = "https://atcoder.jp/contests/" + contest_id + "/tasks/" + task.id + "?lang=en";
  std::string html = http_get(url);
  strip_cr(html);

  std::string content = extract_span_content(html, "lang-en");
  if(content.empty()) {
    content = extract_span_content(html, "lang-ja");
  }
  if(content.empty()) {
    content = extract_task_statement(html);
  }
  if(content.empty()) {
    throw std::runtime_error("no statement content for " + task.id);
  }
  strip_hr(content);

  std::string out;
  out += "<div class=\"header\"><div class=\"title\">" + task.letter + " - " + task.title + "</div>";
  out += "<div class=\"time-limit\"><span class=\"property-title\">time limit</span>" + task.time_limit + "</div>";
  out += "<div class=\"memory-limit\"><span class=\"property-title\">memory limit</span>" + task.memory_limit + "</div></div>\n";
  out += content;
  return out;
}