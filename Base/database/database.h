#pragma once

#include <pqxx/pqxx>
#include <string>
#include <unordered_map>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <format>
#include <fmt/core.h>

class Database {
public:
//    explicit Database(const std::string& config_path = "config.ini");
//    explicit Database(const std::string& connection_string);
    explicit Database();

    int insertDocument(const std::string& url, const std::string& content);
    int insertWord(const std::string& word);
    void insertIndex(int docId, int wordId, int frequency);
    std::vector<std::pair<std::string, int>> searchDocuments(const std::vector<std::string>& queryWords);

private:
    int getWordId(const std::string& word);
    pqxx::connection conn;
};