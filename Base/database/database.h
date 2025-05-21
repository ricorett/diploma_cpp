#pragma once

#include <fmt/core.h>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <format>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <unordered_map>

class Database {
 public:
  explicit Database(const std::string& connection_string);
  int insertDocument(const std::string& url, const std::string& content);
  int insertWord(const std::string& word);
  void insertIndex(int docId, int wordId, int frequency);
  std::vector<std::pair<std::string, int>> searchDocuments(
      const std::vector<std::string>& queryWords);
  void insertWordsWithFrequency(
      int docId, const std::unordered_map<std::string, int>& wordFreq);
  void initializeTables();

 private:
  int getWordId(const std::string& word);
  pqxx::connection conn;
};