#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <boost/locale/encoding_utf.hpp>

class Database {
public:
  Database(const std::string &conn_str);
  void initialize();
  void addDocument(const std::string &url, int depth, const std::map<std::string, int> &word_counts);

private:
  std::unique_ptr<pqxx::connection> conn;
  std::mutex                        db_mutex;
};