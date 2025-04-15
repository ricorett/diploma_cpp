#include "database.h"
#include <iostream>

//Database::Database(const std::string& config_path) {
//    namespace fs = std::filesystem;
//
//    try {
//        if (!fs::exists(config_path)) {
//            throw std::runtime_error("Config file not found: " + config_path);
//        }
//
//        boost::property_tree::ptree pt;
//        boost::property_tree::ini_parser::read_ini(config_path, pt);
//
//        std::string dbname = pt.get<std::string>("database.dbname");
//        std::string user = pt.get<std::string>("database.user");
//        std::string password = pt.get<std::string>("database.password");
//        std::string host = pt.get<std::string>("database.host");
//        int port = pt.get<int>("database.port");
//
//        std::cout << "Connecting to: "
//                  << "dbname=" << dbname
//                  << " user=" << user
//                  << " host=" << host
//                  << " port=" << port << std::endl;
//
//        conn = pqxx::connection(
//                fmt::format("dbname={} user={} password={} host={} port={}",
//                            dbname, user, password, host, port)
//        );
//
//    } catch (const std::exception& e) {
//        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
//        std::exit(1); // Завершаем программу при ошибке
//    }
//}

//Database::Database(const std::string& connection_string) {
//    try {
//        conn = pqxx::connection(connection_string);
//        std::cout << "Connected to: " << conn.dbname() << std::endl;
//    } catch (const std::exception& e) {
//        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
//        std::exit(1);
//    }
//}

Database::Database() {
    try {
        conn = pqxx::connection("user=lana password=1234 dbname=search_db host=localhost port=5432");
        std::cout << "Connected to: " << conn.dbname() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        std::exit(1);
    }
}
int Database::insertDocument(const std::string& url, const std::string& content) {
    try {
        pqxx::work txn(conn);
        auto result = txn.exec("INSERT INTO documents (url, content) VALUES (" +
                               txn.quote(url) + ", " + txn.quote(content) +
                               ") RETURNING id;");
        txn.commit();
        return result[0][0].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return -1;
    }
}

int Database::insertWord(const std::string& word) {
    try {
        pqxx::work txn(conn);
        auto result = txn.exec("INSERT INTO words (word) VALUES (" +
                               txn.quote(word) +
                               ") ON CONFLICT (word) DO NOTHING RETURNING id;");
        txn.commit();
        return result.empty() ? getWordId(word) : result[0][0].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return -1;
    }
}

int Database::getWordId(const std::string& word) {
    try {
        pqxx::work txn(conn);
        auto result = txn.exec("SELECT id FROM words WHERE word = " + txn.quote(word) + ";");
        txn.commit();
        return result.empty() ? -1 : result[0][0].as<int>();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return -1;
    }
}

void Database::insertIndex(int docId, int wordId, int frequency) {
    try {
        pqxx::work txn(conn);
        txn.exec("INSERT INTO index (doc_id, word_id, frequency) VALUES (" +
                 txn.quote(docId) + ", " + txn.quote(wordId) + ", " +
                 txn.quote(frequency) +
                 ") ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = EXCLUDED.frequency;");
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }
}

std::vector<std::pair<std::string, int>> Database::searchDocuments(const std::vector<std::string>& queryWords) {
    std::vector<std::pair<std::string, int>> results;
    if (queryWords.empty()) return results;

    try {
        pqxx::work txn(conn);
        pqxx::result res = txn.exec("SELECT d.url, SUM(idx.frequency) as relevance "
                                    "FROM index idx "
                                    "JOIN words w ON idx.word_id = w.id "
                                    "JOIN documents d ON idx.doc_id = d.id "
                                    "WHERE w.word = ANY(" + txn.quote(queryWords) + ") "
                                                                                    "GROUP BY d.id "
                                                                                    "ORDER BY relevance DESC "
                                                                                    "LIMIT 10;");

        for (const auto& row : res) {
            results.emplace_back(row[0].as<std::string>(), row[1].as<int>());
        }
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    }

    return results;
}