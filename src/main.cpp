#include <cwchar>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <leveldb/db.h>
#include <regex>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using std::wstring, std::string, std::vector;

#ifdef _WIN32
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
inline wstring string_to_wstring(const string &str, int cp = CP_ACP) {
  if (cp != 0 && cp != CP_UTF8)
    return L"";
  int len = MultiByteToWideChar(cp, 0, str.c_str(), (int)str.size(), 0, 0);
  if (len <= 0)
    return L"";
  std::vector<wchar_t> buffer(len);
  int result = MultiByteToWideChar(cp, 0, str.c_str(), (int)str.size(),
                                   buffer.data(), len);
  if (result == 0)
    return L"";
  return wstring(buffer.data(), result);
}
inline string wstring_to_string(const wstring &wstr, int cp = CP_ACP) {
  if (cp != 0 && cp != CP_UTF8)
    return "";
  int len =
      WideCharToMultiByte(cp, 0, wstr.c_str(), (int)wstr.size(), 0, 0, 0, 0);
  if (len <= 0)
    return "";
  vector<char> buffer(len);
  if (WideCharToMultiByte(cp, 0, wstr.c_str(), (int)wstr.size(), buffer.data(),
                          len, 0, 0) == 0) {
    return "";
  }
  return string(buffer.data(), len);
}
#define ACP2UTF8(x) wstring_to_string(string_to_wstring(x, CP_ACP), CP_UTF8)
void SetConsoleColor(int color) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  SetConsoleTextAttribute(hConsole, color);
}
#define COLOR_GREEN FOREGROUND_GREEN
#define COLOR_RED FOREGROUND_RED
#define COLOR_BLUE FOREGROUND_BLUE
#define MSG_WITH_COLOR(msg, color)                                             \
  {                                                                            \
    SetConsoleColor(color);                                                    \
    std::cout << msg;                                                          \
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);      \
  }
#else
inline unsigned int SetConsoleOutputCodePage(unsigned int cp = 65001) {
  return 0;
}
#define ACP2UTF8(x) x
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define MSG_WITH_COLOR(msg, color) std::cout << color << msg << "\033[0m"
#endif
#define MSG_WITH_COLOR_ENDL(msg, color)                                        \
  {                                                                            \
    MSG_WITH_COLOR(msg, color);                                                \
    std::cout << std::endl;                                                    \
  }

auto valuestring() {
  return std::make_shared<cxxopts::values::standard_value<string>>();
}

vector<string> split(const string &str) {
  std::istringstream iss(str);
  vector<string> parts;
  string part;
  while (std::getline(iss, part, '\t')) {
    parts.push_back(part);
  }
  return parts;
}

// East Asian display width helpers (UAX #11 inspired)
static inline bool is_combining_mark(uint32_t u) {
  return (u >= 0x0300 && u <= 0x036F) || (u >= 0x1AB0 && u <= 0x1AFF) ||
         (u >= 0x1DC0 && u <= 0x1DFF) || (u >= 0x20D0 && u <= 0x20FF) ||
         (u >= 0xFE20 && u <= 0xFE2F);
}

static inline bool is_wide_or_fullwidth(uint32_t u) {
  if ((u >= 0x1100 && u <= 0x115F) || (u >= 0x2329 && u <= 0x232A) ||
      (u >= 0x2E80 && u <= 0xA4CF) || (u >= 0xAC00 && u <= 0xD7A3) ||
      (u >= 0xF900 && u <= 0xFAFF) || (u >= 0xFE10 && u <= 0xFE19) ||
      (u >= 0xFE30 && u <= 0xFE6F) || (u >= 0xFF00 && u <= 0xFF60) ||
      (u >= 0xFFE0 && u <= 0xFFE6))
    return true;
  if ((u >= 0x20000 && u <= 0x2FFFD) || (u >= 0x30000 && u <= 0x3FFFD) ||
      (u >= 0x2F800 && u <= 0x2FA1F))
    return true;
  if ((u >= 0x1F300 && u <= 0x1F64F) || (u >= 0x1F680 && u <= 0x1F6FF) ||
      (u >= 0x1F900 && u <= 0x1F9FF) || (u >= 0x1FA70 && u <= 0x1FAFF) ||
      (u >= 0x1F1E6 && u <= 0x1F1FF))
    return true;
  return false;
}

static inline bool is_ambiguous_eaw(uint32_t u) {
  if ((u >= 0x0391 && u <= 0x03A1) || (u >= 0x03A3 && u <= 0x03A9) ||
      (u >= 0x03B1 && u <= 0x03C1) || (u >= 0x03C3 && u <= 0x03C9))
    return true; // Greek
  if (u == 0x0401 || (u >= 0x0410 && u <= 0x044F) || u == 0x0451)
    return true; // Cyrillic common subset
  if ((u >= 0x2010 && u <= 0x2016) || (u >= 0x2018 && u <= 0x2019) ||
      (u >= 0x201C && u <= 0x201D) || (u >= 0x2020 && u <= 0x2022) ||
      (u >= 0x2024 && u <= 0x2027) || u == 0x2030 ||
      (u >= 0x2032 && u <= 0x2033) || u == 0x2035 || u == 0x203B || u == 0x203E)
    return true;
  if (u == 0x20AC)
    return true; // Euro sign
  if ((u >= 0x2190 && u <= 0x2199) || u == 0x21D2 || u == 0x21D4)
    return true; // arrows subset
  if ((u >= 0x2460 && u <= 0x24E9) || (u >= 0x2500 && u <= 0x257F) ||
      (u >= 0x2580 && u <= 0x259F) || (u >= 0x25A0 && u <= 0x25FF) ||
      (u >= 0x2600 && u <= 0x267F) || (u >= 0x2680 && u <= 0x26FF) ||
      (u >= 0x2700 && u <= 0x27BF) || (u >= 0x2B50 && u <= 0x2B59))
    return true;
  return false;
}

static inline int eaw_display_width(const string &s,
                                    bool ambiguous_is_wide = true) {
  const unsigned char *p = reinterpret_cast<const unsigned char *>(s.data());
  const unsigned char *end = p + s.size();
  int width = 0;
  while (p < end) {
    uint32_t cp = 0;
    unsigned char c = *p++;
    if (c < 0x80) {
      cp = c;
    } else if ((c >> 5) == 0x6 && p < end) {
      cp = ((c & 0x1F) << 6) | (*p++ & 0x3F);
    } else if ((c >> 4) == 0xE && (p + 1) <= end) {
      cp = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
      p += 2;
    } else if ((c >> 3) == 0x1E && (p + 2) <= end) {
      cp = ((c & 0x07) << 18) | ((p[0] & 0x3F) << 12) | ((p[1] & 0x3F) << 6) |
           (p[2] & 0x3F);
      p += 3;
    } else {
      width += 1; // invalid sequence fallback
      continue;
    }
    if (is_combining_mark(cp))
      continue;
    if (is_wide_or_fullwidth(cp) ||
        (ambiguous_is_wide && is_ambiguous_eaw(cp))) {
      width += 2;
      continue;
    }
    width += 1;
  }
  return width;
}

class LvlDBUtil {
public:
  LvlDBUtil(string dbpath, bool create_if_missing = false) {
    dbpath_ = expand_user(dbpath).string();
    options_.create_if_missing = create_if_missing;
  }
  ~LvlDBUtil() {}
  static fs::path expand_user(fs::path path) {
    if (!path.empty() && path.string()[0] == '~') {
      assert(path.string().size() == 1 ||
             path.string()[1] ==
                 '/'); // or a check that the ~ isn't part of a filename
      char const *home = std::getenv("HOME");
      if (!home) {                         // In case HOME is not set
        home = std::getenv("USERPROFILE"); // For Windows
      }
      if (home) {
        path.replace_filename(home + path.string().substr(1));
      }
    }
    return path;
  }

  void Worker(const string &pattern = "", bool to_delete = false) {
    leveldb::DB *db_;
    leveldb::Status status = leveldb::DB::Open(options_, dbpath_, &db_);
    if (!status.ok()) {
      throw std::runtime_error("Unable to open database: " + dbpath_);
    }
    leveldb::Iterator *it_ = db_->NewIterator(leveldb::ReadOptions());

    vector<string> col1, col2, colV;
    vector<bool> deletedFlags;

    const std::regex metaRe("^\x01.*$");
    std::regex patternRe;
    bool usePattern = !pattern.empty();
    if (usePattern)
      patternRe = std::regex(pattern);

    for (it_->SeekToFirst(); it_->Valid(); it_->Next()) {
      string key = it_->key().ToString();
      if (key.empty() || std::regex_match(key, metaRe))
        continue;
      string value = it_->value().ToString();
      bool match = !usePattern || std::regex_search(key, patternRe) ||
                   std::regex_search(value, patternRe);
      if (!match)
        continue;
      auto parts = split(key);
      string p1 = parts.size() ? parts[0] : "";
      string p2 = parts.size() > 1 ? parts[1] : "";
      bool delOk = false;
      if (to_delete) {
        leveldb::Status ds = db_->Delete(leveldb::WriteOptions(), key);
        delOk = ds.ok();
      }
      col1.push_back(p1);
      col2.push_back(p2);
      colV.push_back(value);
      deletedFlags.push_back(delOk);
    }

    // use global eaw_display_width for visual width
    int w1 = 0, w2 = 0, wV = 0;
#define MAX(a, b) ((a > b) ? a : b)
    for (size_t i = 0; i < col1.size(); ++i) {
      w1 = MAX(w1, eaw_display_width(col1[i]));
      w2 = MAX(w2, eaw_display_width(col2[i]));
      wV = MAX(wV, eaw_display_width(colV[i]));
    }

#define MSG_WITH_COLOR_FMT(s, width, color)                                    \
  {                                                                            \
    MSG_WITH_COLOR(s, color);                                                  \
    int vw = eaw_display_width(s);                                             \
    if (vw < width)                                                            \
      std::cout << string(width - vw, ' ');                                    \
  }
    for (size_t i = 0; i < col1.size(); ++i) {
      if (deletedFlags[i])
        std::cout << "deleted: ";
      MSG_WITH_COLOR_FMT(col1[i], w1, COLOR_BLUE);
      std::cout << "  ";
      MSG_WITH_COLOR_FMT(col2[i], w2, COLOR_GREEN);
      std::cout << "  ";
      MSG_WITH_COLOR_ENDL(colV[i], COLOR_RED);
    }
    if (it_)
      delete it_;
    if (db_)
      delete db_;
  }

private:
  leveldb::Options options_;
  string dbpath_;
};

int main(int argc, char *argv[]) {
  unsigned int code = SetConsoleOutputCodePage(65001);
  try {
    cxxopts::Options options("lvldbutil",
                             "- a tool to play with user dict of rime");
    options.add_options()("p,path", "path of database", valuestring())(
        "d,delete", "delete a key with pattern, -P pat")(
        "l,list", "list user dicts, if pattern provided, list with pattern")(
        "P,pattern", "pattern to query", valuestring())("h,help", "print help");
    auto result = options.parse(argc, argv);
    if (argc == 1 || result.count("help")) {
      std::cout << options.help() << std::endl;
      SetConsoleOutputCodePage(code);
      return 0;
    }
    string dbpath;
    if (result.count("path")) {
      dbpath = result["path"].as<string>();
      dbpath = ACP2UTF8(dbpath);
      if (!fs::exists(dbpath) || dbpath.empty()) {
        std::cerr << "database path not exist: " << dbpath << std::endl;
        return 1;
      }
    }
    string pattern;
    if (result.count("pattern")) {
      pattern = result["pattern"].as<string>();
      pattern = ACP2UTF8(pattern);
    }

    LvlDBUtil dbutil(dbpath);

    if (result.count("list")) {
      dbutil.Worker(pattern);
    } else if (result.count("delete")) {
      dbutil.Worker(pattern, true);
    }
  } catch (const std::exception &e) {
    std::cerr << "error parsing options: " << e.what() << std::endl;
    SetConsoleOutputCodePage(code);
    return 1;
  }
  SetConsoleOutputCodePage(code);
  return 0;
}
