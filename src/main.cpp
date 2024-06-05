#include <leveldb/db.h>
#include <iostream>
#include <filesystem>
#include <regex>
#include <string>
#include <sstream>
#include <vector>
#include "cxxopts.hpp"

namespace fs = std::filesystem;

#ifdef _WIN32
#define _UNICODE
#define UNICODE
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
inline std::wstring string_to_wstring(const std::string& str,
                                      int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return L"";
  // calc len
  int len =
      MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), NULL, 0);
  if (len <= 0)
    return L"";
  std::wstring res;
  TCHAR* buffer = new TCHAR[len + 1];
  MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), buffer, len);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}
inline std::string wstring_to_string(const std::wstring& wstr,
                                     int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return "";
  int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(),
                                NULL, 0, NULL, NULL);
  if (len <= 0)
    return "";
  std::string res;
  char* buffer = new char[len + 1];
  WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(), buffer, len,
                      NULL, NULL);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}
#define ACP2UTF8(x)  wstring_to_string(string_to_wstring(x, CP_ACP), CP_UTF8)

#else
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = 65001) {
  return 0;
}
#define ACP2UTF8(x)  x

#endif /* _WIN32 */

// for console text color in win32
#ifdef _WIN32
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
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_RED "\033[34m"
#define MSG_WITH_COLOR(msg, color) std::cout << color << msg << "\033[0m"
#endif
#define MSG_WITH_COLOR_ENDL(msg, color)                                        \
  {                                                                            \
    MSG_WITH_COLOR(msg, color);                                                \
    std::cout << std::endl;                                                    \
  }

auto valuestring()
{ return std::make_shared<cxxopts::values::standard_value<std::string>>(); }


std::vector<std::string> split(const std::string& str) {
  std::istringstream iss(str);
  std::vector<std::string> parts;
  std::string part;
  while (std::getline(iss, part, '\t')) {
    parts.push_back(part);
  }
  return parts;
}

// 现在，parts[0]包含"part1"，parts[1]包含"part2"
class LvlDBUtil {
  public:
    LvlDBUtil(std::string dbpath, bool create_if_missing = false) {
      dbpath_ = expand_user(dbpath).string();
      options_.create_if_missing = create_if_missing;
    }
    ~LvlDBUtil(){}
    static fs::path expand_user(fs::path path) {
      if (!path.empty() && path.string()[0] == '~') {
        assert(path.string().size() == 1 || path.string()[1] == '/');  // or a check that the ~ isn't part of a filename
        char const* home = std::getenv("HOME");
        if (!home) { // In case HOME is not set
          home = std::getenv("USERPROFILE"); // For Windows
        }
        if (home) {
          path.replace_filename(home + path.string().substr(1));
        }
      }
      return path;
    }

    void Worker(const std::string& pattern = "", bool to_delete=false) {
      leveldb::DB* db_;
      leveldb::Status status = leveldb::DB::Open(options_, dbpath_, &db_);
      if (!status.ok()) {
        throw std::runtime_error("Unable to open database: " + dbpath_ );
      }
      leveldb::Iterator* it_ = db_->NewIterator(leveldb::ReadOptions());
      for(it_->SeekToFirst(); it_->Valid(); it_->Next()) {
        auto parts = split(it_->key().ToString());
        auto printinfo = [this, parts, it_](){
          MSG_WITH_COLOR(parts.at(0), COLOR_BLUE);
          if(parts.size() == 2) {
            std::cout << "\t";
            MSG_WITH_COLOR(parts.at(1), COLOR_GREEN);
          }
          std::cout << "\t";
          MSG_WITH_COLOR_ENDL(it_->value().ToString(), COLOR_RED);
        };
        if (pattern.empty()) {
          printinfo();
        }
        else if(to_delete) {
          if (std::regex_search(it_->key().ToString(), std::regex(pattern))) {
            db_->Delete(leveldb::WriteOptions(), it_->key().ToString());
            std::cout << "deleted: ";
            printinfo();
          } 
        } else {
          if (std::regex_search(it_->key().ToString(), std::regex(pattern)))
            printinfo();
        }
      }
      if (it_)
        delete it_;
      if (db_)
        delete db_;
    }
  private:
    leveldb::Options options_;
    std::string dbpath_;
};


int main(int argc, char* argv[]){
	unsigned int code = SetConsoleOutputCodePage(65001);
  try {
    cxxopts::Options options("lvldbutil", "- a tool to play with user dict of rime");
    options.add_options()
      ("p,path", "path of database", valuestring())
      ("q,query", "query a pattern")
      ("d,delete", "delete a key with pattern, -P pat")
      ("l,list", "list all user dicts")
      ("P,pattern", "pattern to query", valuestring())
      ("h,help", "print help");
    auto result = options.parse(argc, argv);
    if (argc == 1 || result.count("help")) {
      std::cout << options.help() << std::endl;
      return 0;
    }
    std::string dbpath;
    if (result.count("path")) {
      dbpath = result["path"].as<std::string>();
      dbpath = ACP2UTF8(dbpath);
      if (!fs::exists(dbpath) || dbpath.empty()) {
        std::cerr << "database path not exist: " << dbpath << std::endl;
        return 1;
      }
    }
    std::string pattern;
    if (result.count("pattern")) {
      pattern = result["pattern"].as<std::string>();
      pattern = ACP2UTF8(pattern);
    }

    LvlDBUtil dbutil (dbpath);

    if (result.count("list")) {
      dbutil.Worker();
      return 0;
    } else if (result.count("query")) {
      if (!pattern.empty()) {
        dbutil.Worker(pattern);
      } else {
        dbutil.Worker();
      }
      return 0;
    } else if (result.count("delete")) {
      if (!pattern.empty()) {
        dbutil.Worker(pattern, true);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "error parsing options: " << e.what() << std::endl;
		SetConsoleOutputCodePage(code);
    return 1;
  }

	SetConsoleOutputCodePage(code);
  return 0;
}
