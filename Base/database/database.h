#pragma once

#include <pqxx/pqxx>
#include <string>
#include <unordered_map>

class Database {
public:
    Database(const std::string& dbname, const std::string& user,
             const std::string& password, const std::string& host, int port);

    void initializeTables();
    int insertDocument(const std::string& url, const std::string& content);
    int insertWord(const std::string& word);
    void insertIndex(int docId, int wordId, int frequency);

private:
    pqxx::connection conn;
};