#include "database.h"

#include <iostream>

Database::Database(const std::string& conn_str) : conn_(conn_str) {}


void Database::initialize_tables() {
    try {
        pqxx::work txn(conn_);

        txn.exec(
            "CREATE TABLE IF NOT EXISTS documents ("
            "id SERIAL PRIMARY KEY, "
            "url TEXT UNIQUE NOT NULL, "
            "content TEXT NOT NULL);"

            "CREATE TABLE IF NOT EXISTS words ("
            "id SERIAL PRIMARY KEY, "
            "word TEXT UNIQUE NOT NULL);"

            "CREATE TABLE IF NOT EXISTS inverted_index ("
            "doc_id INT REFERENCES documents(id) ON DELETE CASCADE, "
            "word_id INT REFERENCES words(id) ON DELETE CASCADE, "
            "frequency INT NOT NULL, "
            "PRIMARY KEY(doc_id, word_id));"
        );

        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database initialization error: " << e.what() << std::endl;
        throw;
    }
}

int Database::insert_document(const std::string& url, const std::string& content) {
    try {
        pqxx::work txn(conn_);

        // Формируем параметризованный запрос с экранированием
        const std::string sql =
            "INSERT INTO documents (url, content) "
            "VALUES (" + txn.quote(url) + ", " + txn.quote(content) + ") "
            "ON CONFLICT (url) DO NOTHING RETURNING id";

        pqxx::result res = txn.exec(sql);

        if (res.empty()) {
            std::cout << "[DB] Document exists: " << url << "\n";
            return -1;
        }

        const int doc_id = res[0][0].as<int>();
        txn.commit();
        return doc_id;
    } catch (const std::exception& e) {
        std::cerr << "[DB ERROR] Insert document: " << e.what() << "\n";
        return -1;
    }
}

void Database::insert_words_frequency(int doc_id,
                                    const std::unordered_map<std::string, int>& words) {
    try {
        pqxx::work txn(conn_);
        std::unordered_map<std::string, int> freq;
        for (const auto& [key, value] : words) {
            freq[key] += value; // Суммирование значений
        }

        for (const auto& [word, count] : freq) {
            const int word_id = get_word_id(txn, word);

            // Используем exec вместо exec_params
            txn.exec(
                "INSERT INTO inverted_index (doc_id, word_id, frequency) "
                "VALUES (" +
                txn.quote(doc_id) + ", " +
                txn.quote(word_id) + ", " +
                txn.quote(count) + ") " +
                "ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = " +
                txn.quote(count)
            );
        }
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "[DB ERROR] Insert words: " << e.what() << "\n";
    }
}

// database/database.cpp
//
// std::string make_sql_array(const std::vector<std::string>& terms, pqxx::transaction_base& txn) {
//     std::string array_str = "'{";
//     bool first = true;
//     for (const auto& term : terms) {
//         if (!first) array_str += ",";
//         array_str += txn.esc(term);
//         first = false;
//     }
//     array_str += "}'";
//     return array_str;
// }

std::vector<std::pair<std::string, int>> Database::search(const std::vector<std::string>& terms) {
    std::vector<std::pair<std::string, int>> results;

    // Фильтрация пустых терминов
    std::vector<std::string> valid_terms;
    for (const auto& term : terms) {
        if (!term.empty()) valid_terms.push_back(term);
    }
    if (valid_terms.empty()) return results;

    try {
        pqxx::work txn(conn_);

        // 1. Формируем массив с экранированием
        std::vector<std::string> quoted_terms;
        for (const auto& term : valid_terms) {
            quoted_terms.push_back(txn.quote(term));
        }

        // 2. Собираем SQL-массив
        const std::string terms_array =
            "ANY(ARRAY[" + boost::algorithm::join(quoted_terms, ",") + "]::text[])";

        // 3. Формируем запрос
        const std::string sql =
            "SELECT d.url, SUM(idx.frequency) AS relevance "
            "FROM inverted_index idx "
            "JOIN words w ON idx.word_id = w.id "
            "JOIN documents d ON idx.doc_id = d.id "
            "WHERE w.word = " + terms_array + " "
            "GROUP BY d.url "
            "ORDER BY relevance DESC "
            "LIMIT 10";

        std::cout << "[DEBUG] SQL: " << sql << "\n"; // Для отладки

        pqxx::result res = txn.exec(sql);

        for (const auto& row : res) {
            results.emplace_back(
                row["url"].as<std::string>(),
                row["relevance"].as<int>()
            );
        }

        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "[DB ERROR] Search: " << e.what() << "\n";
    }
    return results;
}

int Database::get_word_id(pqxx::work& txn, const std::string& word) {
    try {
        // Вставка слова с обработкой конфликтов
        pqxx::result res = txn.exec(
            "INSERT INTO words (word) VALUES (" + txn.quote(word) + ") "
            "ON CONFLICT (word) DO NOTHING RETURNING id"
        );

        if (!res.empty()) {
            return res[0][0].as<int>();
        }

        // Если слово уже существует, получаем его ID
        res = txn.exec(
            "SELECT id FROM words WHERE word = " + txn.quote(word)
        );

        if (!res.empty()) {
            return res[0][0].as<int>();
        }

        throw std::runtime_error("Word not found after insertion attempt");

    } catch (const std::exception& e) {
        std::cerr << "[DB ERROR] get_word_id: " << e.what() << std::endl;
        return -1;
    }
}