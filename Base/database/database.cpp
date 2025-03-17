#include "database.h"
#include <iostream>

Database::Database(const std::string& dbname, const std::string& user,
                   const std::string& password, const std::string& host, int port)
        : conn("dbname=" + dbname + " user=" + user + " password=" + password +
               " host=" + host + " port=" + std::to_string(port)) {}

void Database::initializeTables() {
    pqxx::work txn(conn);
    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS documents (
            id SERIAL PRIMARY KEY,
            url TEXT UNIQUE NOT NULL,
            content TEXT NOT NULL
        );
        CREATE TABLE IF NOT EXISTS words (
            id SERIAL PRIMARY KEY,
            word TEXT UNIQUE NOT NULL
        );
        CREATE TABLE IF NOT EXISTS index (
            doc_id INT REFERENCES documents(id) ON DELETE CASCADE,
            word_id INT REFERENCES words(id) ON DELETE CASCADE,
            frequency INT NOT NULL,
            PRIMARY KEY (doc_id, word_id)
        );
    )");
    txn.commit();
}

int Database::insertDocument(const std::string& url, const std::string& content) {
    pqxx::work txn(conn);
    auto result = txn.exec_params("INSERT INTO documents (url, content) VALUES ($1, $2) "
                                  "ON CONFLICT (url) DO NOTHING RETURNING id;", url, content);
    txn.commit();
    return result.empty() ? -1 : result[0][0].as<int>();
}

int Database::insertWord(const std::string& word) {
    pqxx::work txn(conn);
    auto result = txn.exec_params("INSERT INTO words (word) VALUES ($1) "
                                  "ON CONFLICT (word) DO NOTHING RETURNING id;", word);
    txn.commit();
    return result.empty() ? -1 : result[0][0].as<int>();
}

void Database::insertIndex(int docId, int wordId, int frequency) {
    pqxx::work txn(conn);
    txn.exec_params("INSERT INTO index (doc_id, word_id, frequency) VALUES ($1, $2, $3) "
                    "ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = $3;", docId, wordId, frequency);
    txn.commit();
}