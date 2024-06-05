#include <leveldb/db.h>
#include <iostream>
#include <filesystem>
#include <regex>
#include "cxxopts.hpp"

namespace fs = std::filesystem;

#ifdef _WIN32
#include <windows.h>
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
#else
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = 65001) {
  return 0;
}
#endif /* _WIN32 */

auto valuestring()
{ return std::make_shared<cxxopts::values::standard_value<std::string>>(); }

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
        if (pattern.empty())
          std::cout << "Key: " << it_->key().ToString()
            << ", Value: " << it_->value().ToString() << std::endl;
        else if(to_delete) {
          if (std::regex_search(it_->key().ToString(), std::regex(pattern))) {
            db_->Delete(leveldb::WriteOptions(), it_->key().ToString());
            std::cout << "Delete key: " << it_->key().ToString()
              << ", value: " << it_->value().ToString() << std::endl;
          } 
        } else {
          if (std::regex_search(it_->key().ToString(), std::regex(pattern)))
            std::cout << "Key: " << it_->key().ToString()
              << ", Value: " << it_->value().ToString() << std::endl;
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
      ("l,list", "list all user directories")
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
      if (!fs::exists(dbpath) || dbpath.empty()) {
        std::cerr << "database path not exist: " << dbpath << std::endl;
        return 1;
      }
    }
    std::string pattern;
    if (result.count("pattern")) {
      pattern = result["pattern"].as<std::string>();
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
