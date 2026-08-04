#include "convert.hpp"
#include "../http/http.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

  struct Node {
    std::string tag;
    std::unordered_map<std::string, std::string> attrs;
    std::string text;
    std::vector<Node> children;

    bool is_text() const {
      return tag.empty();
    }
  };

  std::string lowercase(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    for(char c : s) {
      r += (char)std::tolower((unsigned char)c);
    }
    return r;
  }

  bool self_closing(const std::string &tag) {
    static const std::unordered_set<std::string> tags = {
      "area", "base", "br", "col", "embed", "hr", "img", "input",
      "link", "meta", "param", "source", "track", "wbr"
    };
    return tags.count(tag) > 0;
  }

  void decode_entities(std::string &s) {
    static const std::unordered_map<std::string, std::string> map = {
      {"&nbsp;", " "},   {"&lt;", "<"},     {"&gt;", ">"},       {"&amp;", "&"},
      {"&quot;", "\""},  {"&apos;", "'"},   {"&le;", "\u2264"},   {"&ge;", "\u2265"},
      {"&ne;", "\u2260"},{"&minus;", "\u2212"}, {"&times;", "\u00d7"}, {"&divide;", "\u00f7"},
      {"&infin;", "\u221e"}, {"&rarr;", "\u2192"}, {"&larr;", "\u2190"}, {"&harr;", "\u2194"},
      {"&uarr;", "\u2191"}, {"&darr;", "\u2193"}, {"&hellip;", "\u2026"}, {"&mdash;", "\u2014"},
      {"&ndash;", "\u2013"}, {"&laquo;", "\u00ab"}, {"&raquo;", "\u00bb"}, {"&lowbar;", "_"},
      {"&prime;", "\u2032"}, {"&deg;", "\u00b0"}, {"&plusmn;", "\u00b1"}, {"&mp;", "\u2213"},
      {"&cdot;", "\u22c5"}, {"&cdots;", "\u22ef"}, {"&ldots;", "\u2026"}, {"&sum;", "\u2211"},
      {"&prod;", "\u220f"}, {"&in;", "\u2208"}, {"&notin;", "\u2209"}, {"&subset;", "\u2282"},
      {"&supset;", "\u2283"}, {"&sube;", "\u2286"}, {"&supe;", "\u2287"}, {"&cap;", "\u2229"},
      {"&cup;", "\u222a"}, {"&oplus;", "\u2295"}, {"&ominus;", "\u2296"}, {"&otimes;", "\u2297"},
      {"&forall;", "\u2200"}, {"&exists;", "\u2203"}, {"&neg;", "\u00ac"}, {"&empty;", "\u2205"},
      {"&partial;", "\u2202"}, {"&nabla;", "\u2207"}, {"&int;", "\u222b"}, {"&iff;", "\u21d4"},
      {"&equiv;", "\u2261"}, {"&approx;", "\u2248"}, {"&sim;", "\u223c"}, {"&cong;", "\u2245"},
      {"&prop;", "\u221d"}, {"&to;", "\u2192"}, {"&gets;", "\u2190"}, {"&mapsto;", "\u21a6"},
      {"&langle;", "\u27e8"}, {"&rangle;", "\u27e9"}, {"&lceil;", "\u2308"}, {"&rceil;", "\u2309"},
      {"&lfloor;", "\u230a"}, {"&rfloor;", "\u230b"}, {"&mid;", "|"}, {"&aleph;", "\u2135"}
    };

    std::string out;
    out.reserve(s.size());
    for(size_t i = 0; i < s.size();) {
      if(s[i] == '&') {
        size_t semi = s.find(';', i);
        if(semi != std::string::npos && semi - i <= 12) {
          std::string ent = s.substr(i, semi - i + 1);
          auto it = map.find(ent);
          if(it != map.end()) {
            out += it->second;
            i = semi + 1;
            continue;
          }
          if(ent.size() > 2 && ent[1] == '#') {
            int cp = 0;
            if(ent[2] == 'x' || ent[2] == 'X') {
              cp = std::stoi(ent.substr(3, ent.size() - 4), nullptr, 16);
            } else {
              cp = std::stoi(ent.substr(2, ent.size() - 3));
            }
            if(cp < 0x80) {
              out += (char)cp;
            } else if(cp < 0x800) {
              out += (char)(0xC0 | (cp >> 6));
              out += (char)(0x80 | (cp & 0x3F));
            } else if(cp < 0x10000) {
              out += (char)(0xE0 | (cp >> 12));
              out += (char)(0x80 | ((cp >> 6) & 0x3F));
              out += (char)(0x80 | (cp & 0x3F));
            } else {
              out += (char)(0xF0 | (cp >> 18));
              out += (char)(0x80 | ((cp >> 12) & 0x3F));
              out += (char)(0x80 | ((cp >> 6) & 0x3F));
              out += (char)(0x80 | (cp & 0x3F));
            }
            i = semi + 1;
            continue;
          }
        }
      }
      out += s[i++];
    }
    s = std::move(out);
  }

  Node parse_html(const std::string &html) {
    Node root;
    std::vector<Node*> stack;
    stack.push_back(&root);

    size_t pos = 0;
    const size_t n = html.size();

    while(pos < n) {
      if(html[pos] != '<') {
        size_t next = html.find('<', pos);
        if(next == std::string::npos) {
          next = n;
        }
        std::string text = html.substr(pos, next - pos);
        decode_entities(text);
        Node tnode;
        tnode.text = std::move(text);
        stack.back()->children.push_back(std::move(tnode));
        pos = next;
        continue;
      }

      size_t end = html.find('>', pos);
      if(end == std::string::npos) {
        break;
      }

      if(html.compare(pos, 4, "<!--") == 0) {
        size_t cc = html.find("-->", pos + 4);
        if(cc == std::string::npos) {
          break;
        }
        pos = cc + 3;
        continue;
      }
      if(html.compare(pos, 2, "<!") == 0) {
        pos = end + 1;
        continue;
      }
      if(html.compare(pos, 2, "<?") == 0) {
        pos = end + 1;
        continue;
      }

      std::string tok = html.substr(pos + 1, end - pos - 1);
      pos = end + 1;

      if(!tok.empty() && tok[0] == '/') {
        std::string name;
        size_t i = 1;
        while(i < tok.size() && !std::isspace((unsigned char)tok[i])) {
          name += tok[i++];
        }
        name = lowercase(name);
        if(!name.empty()) {
          for(auto it = stack.rbegin(); it != stack.rend(); ++it) {
            if((*it)->tag == name) {
              stack.resize(stack.size() - (std::distance(stack.rbegin(), it) + 1));
              break;
            }
          }
        }
        continue;
      }

      std::string name;
      size_t i = 0;
      while(i < tok.size() && !std::isspace((unsigned char)tok[i])) {
        name += tok[i++];
      }
      name = lowercase(name);
      if(name.empty()) {
        continue;
      }

      Node node;
      node.tag = name;
      while(i < tok.size()) {
        while(i < tok.size() && (std::isspace((unsigned char)tok[i]) || tok[i] == '/')) {
          i++;
        }
        if(i >= tok.size()) {
          break;
        }
        std::string key;
        while(i < tok.size() && !std::isspace((unsigned char)tok[i]) && tok[i] != '=' && tok[i] != '/') {
          key += tok[i++];
        }
        while(i < tok.size() && (std::isspace((unsigned char)tok[i]) || tok[i] == '=')) {
          i++;
        }
        std::string value;
        if(i < tok.size() && (tok[i] == '"' || tok[i] == '\'')) {
          char q = tok[i++];
          while(i < tok.size() && tok[i] != q) {
            value += tok[i++];
          }
          if(i < tok.size()) {
            i++;
          }
        } else {
          while(i < tok.size() && !std::isspace((unsigned char)tok[i]) && tok[i] != '/') {
            value += tok[i++];
          }
        }
        node.attrs[lowercase(key)] = value;
      }

      bool selfcl = self_closing(node.tag);
      if(!tok.empty() && tok.back() == '/') {
        selfcl = true;
      }

      stack.back()->children.push_back(std::move(node));
      if(!selfcl) {
        stack.push_back(&stack.back()->children.back());
      }
    }

    return root;
  }

  std::string escape_text(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    for(char c : s) {
      switch(c) {
      case '\\': case '`': case '*': case '_': case '[': case ']': case '#': case '<': case '>':
        r += '\\';
        r += c;
        break;
      default:
        r += c;
      }
    }
    return r;
  }

  void collect_text(const Node &node, std::string &out, bool raw) {
    if(node.is_text()) {
      out += raw ? node.text : escape_text(node.text);
      return;
    }
    if(node.tag == "script" || node.tag == "style") {
      return;
    }
    if(node.tag == "br") {
      out += ' ';
      return;
    }
    if(node.tag == "sub") {
      out += '_';
      for(const Node &c : node.children) {
        collect_text(c, out, raw);
      }
      return;
    }
    if(node.tag == "sup") {
      out += '^';
      for(const Node &c : node.children) {
        collect_text(c, out, raw);
      }
      return;
    }
    char scripted = 0;
    auto it = node.attrs.find("class");
    if(it != node.attrs.end()) {
      const std::string &c = it->second;
      if(c.find("MJXp-msubsup") != std::string::npos) {
        scripted = '_';
      } else if(c.find("MJXp-msup") != std::string::npos || c.find("MJXp-munderover") != std::string::npos) {
        scripted = '^';
      }
    }
    bool first = true;
    for(const Node &c : node.children) {
      if(scripted && !first) {
        out += scripted;
      }
      collect_text(c, out, raw);
      first = false;
    }
  }

  struct RenderCtx {
    bool in_pre = false;
    bool in_header = false;
    bool in_sample_io = false;
    int list_depth = 0;
  };

  class Renderer {
  public:
    explicit Renderer(const std::filesystem::path &files_dir) : files_dir_(files_dir) {}

    std::string run(const Node &root) {
      RenderCtx ctx;
      for(const Node &c : root.children) {
        render(c, ctx);
      }
      return out_.str();
    }

  private:
    std::ostringstream out_;
    std::filesystem::path files_dir_;

    bool has_class(const Node &node, const std::string &needle) const {
      auto it = node.attrs.find("class");
      if(it == node.attrs.end()) {
        return false;
      }
      return it->second.find(needle) != std::string::npos;
    }

    void render(const Node &node, RenderCtx ctx) {
      if(node.is_text()) {
        if(ctx.in_pre) {
          out_ << node.text;
        } else {
          out_ << escape_text(node.text);
        }
        return;
      }

      const std::string &tag = node.tag;

      if(tag == "script" || tag == "style" || tag == "head" || tag == "noscript" || tag == "nobr") {
        return;
      }

      std::string cls;
      auto it = node.attrs.find("class");
      if(it != node.attrs.end()) {
        cls = it->second;
      }

      if(cls.contains("MathJax") && !cls.contains("Preview")) {
        return;
      }
      if(cls.contains("input-output-copier")) {
        return;
      }

      if(ctx.in_pre) {
        if(tag == "br") {
          out_ << '\n';
          return;
        }
        if(tag == "div" && cls.contains("test-example-line")) {
          for(const Node &c : node.children) {
            render(c, ctx);
          }
          out_ << '\n';
        } else {
          for(const Node &c : node.children) {
            render(c, ctx);
          }
        }
        return;
      }

      if(tag == "div" && cls.contains("header")) {
        RenderCtx h = ctx;
        h.in_header = true;
        for(const Node &c : node.children) {
          render(c, h);
        }
        out_ << '\n';
        return;
      }

      if(ctx.in_header) {
        if(tag == "div" && cls.contains("title")) {
          std::string t;
          collect_text(node, t, true);
          out_ << "# " << t << "\n\n";
          return;
        }
        if(tag == "div" && (cls.contains("time-limit") || cls.contains("memory-limit") ||
                            cls.contains("input-file") || cls.contains("output-file"))) {
          std::string label, value;
          for(const Node &c : node.children) {
            if(!c.is_text() && has_class(c, "property-title")) {
              collect_text(c, label, false);
            } else {
              collect_text(c, value, false);
            }
          }
          out_ << "**" << label << ":**" << value << "\n";
          return;
        }
      }

      if(tag == "div" && cls.contains("section-title")) {
        std::string t;
        collect_text(node, t, true);
        out_ << "\n## " << t << "\n\n";
        return;
      }

      if(tag == "div" && (cls == "input" || cls == "output")) {
        out_ << "\n**" << (cls == "input" ? "Input" : "Output") << "**\n\n";
        RenderCtx io = ctx;
        io.in_sample_io = true;
        for(const Node &c : node.children) {
          if(!c.is_text() && c.tag == "div" && has_class(c, "title")) {
            continue;
          }
          render(c, io);
        }
        return;
      }

      if(tag == "p") {
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        out_ << "\n\n";
        return;
      }

      if(tag == "pre") {
        out_ << "\n```\n";
        RenderCtx p = ctx;
        p.in_pre = true;
        for(const Node &c : node.children) {
          render(c, p);
        }
        out_ << "```\n\n";
        return;
      }

      if(tag == "ul" || tag == "ol") {
        RenderCtx l = ctx;
        l.list_depth++;
        for(const Node &c : node.children) {
          render(c, l);
        }
        out_ << "\n";
        return;
      }

      if(tag == "li") {
        if(ctx.list_depth > 1) {
          out_ << std::string((size_t)(ctx.list_depth - 1) * 2, ' ');
        }
        out_ << "- ";
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        out_ << "\n";
        return;
      }

      if(tag == "br") {
        out_ << "\n";
        return;
      }

      if(tag == "hr") {
        out_ << "\n---\n";
        return;
      }

      if(tag == "img") {
        render_image(node);
        return;
      }

      if(tag == "a") {
        out_ << "[";
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        out_ << "]";
        auto hit = node.attrs.find("href");
        if(hit != node.attrs.end()) {
          out_ << "(" << hit->second << ")";
        }
        return;
      }

      if(tag == "sub") {
        out_ << "_";
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        return;
      }

      if(tag == "sup") {
        out_ << "^";
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        return;
      }

      if(tag == "span" && (cls.contains("tex-span") || cls.contains("MathJax_Preview") || cls == "math")) {
        std::string m;
        for(const Node &c : node.children) {
          collect_text(c, m, true);
        }
        out_ << "`" << m << "`";
        return;
      }

      if(tag == "b" || tag == "strong" || cls.contains("tex-font-style-bf")) {
        out_ << "**";
        for(const Node &c : node.children)  {
          render(c, ctx);
        }
        out_ << "**";
        return;
      }

      if(tag == "i" || tag == "em" || cls.contains("tex-font-style-it")) {
        out_ << "*";
        for(const Node &c : node.children) {
          render(c, ctx);
        }
        out_ << "*";
        return;
      }

      if(tag == "tt" || tag == "code" || cls.contains("tex-font-style-tt") ||
         cls.contains("kwd") || cls.contains("pln") || cls.contains("pun") || cls.contains("lit")) {
        std::string m;
        for(const Node &c : node.children) {
          collect_text(c, m, true);
        }
        out_ << "`" << m << "`";
        return;
      }

      if(tag.size() == 2 && tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '6') {
        std::string t;
        collect_text(node, t, true);
        out_ << std::string((size_t)(tag[1] - '0'), '#') << " " << t << "\n\n";
        return;
      }

      for(const Node &c : node.children) {
        render(c, ctx);
      }
    }

    void render_image(const Node &node) {
      auto sit = node.attrs.find("src");
      if(sit == node.attrs.end()) {
        return;
      }
      std::string src = sit->second;
      std::string alt;
      auto ait = node.attrs.find("alt");
      if(ait != node.attrs.end()) {
        alt = ait->second;
      }

      if(src.starts_with("http://") || src.starts_with("https://")) {
        std::string name = src.substr(src.find_last_of('/') + 1);
        if(name.empty()) {
          name = "image";
        }
        std::filesystem::path local = files_dir_ / name;
        if(!std::filesystem::exists(local)) {
          std::filesystem::create_directories(files_dir_);
          if(!download_image(src, local)) {
            out_ << "![" << alt << "](" << src << ")";
            return;
          }
        }
        out_ << "![" << alt << "](statement_files/" << name << ")";
        return;
      }

      out_ << "![" << alt << "](" << src << ")";
    }

    bool download_image(const std::string &url, const std::filesystem::path &dest) {
      static std::unordered_map<std::string, std::filesystem::path> cache;
      auto it = cache.find(url);
      if(it != cache.end()) {
        std::error_code ec;
        std::filesystem::copy_file(it->second, dest, std::filesystem::copy_options::overwrite_existing, ec);
        return !ec;
      }
      try {
        std::string data = http_get(url);
        if(data.empty()) {
          return false;
        }
        std::ofstream fout(dest, std::ios::binary);
        fout.write(data.data(), (std::streamsize)data.size());
        fout.close();
        if(!fout.good()) {
          std::filesystem::remove(dest);
          return false;
        }
        cache[url] = dest;
        return true;
      } catch(...) {
        return false;
      }
    }
  };
}

std::string statement_html_to_md(const std::string &html, const std::filesystem::path &statement_files_dir) {
  Node root = parse_html(html);
  Renderer renderer(statement_files_dir);
  return renderer.run(root);
}