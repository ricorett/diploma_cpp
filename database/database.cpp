#include "database.hpp"
#include <iostream>

Database::Database(const std::string &conn_str) {
  conn = std::make_unique<pqxx::connection>(conn_str);
  if (!conn->is_open()) {
    throw std::runtime_error("Database connection failed");
  }
}

void Database::initialize() {
  std::lock_guard<std::mutex> lock(db_mutex);
  pqxx::work                  tx(*conn);

  tx.exec("CREATE TABLE IF NOT EXISTS documents ("
          "    id SERIAL PRIMARY KEY,"
          "    url TEXT UNIQUE NOT NULL,"
          "    depth INTEGER NOT NULL"
          ");"

          "CREATE TABLE IF NOT EXISTS words ("
          "    id SERIAL PRIMARY KEY,"
          "    word TEXT UNIQUE NOT NULL"
          ");"

          "CREATE TABLE IF NOT EXISTS document_word ("
          "    document_id INTEGER REFERENCES documents(id) ON DELETE CASCADE,"
          "    word_id INTEGER REFERENCES words(id) ON DELETE CASCADE,"
          "    count INTEGER NOT NULL,"
          "    PRIMARY KEY (document_id, word_id)"
          ");");

  tx.commit();
}

void Database::addDocument(const std::string &url, int depth, const std::map<std::string, int> &word_counts) {
  std::lock_guard<std::mutex> lock(db_mutex);
  pqxx::work                  tx(*conn);

  try {

    pqxx::result doc_res = tx.exec_params("INSERT INTO documents (url, depth) VALUES ($1, $2) "
                                          "ON CONFLICT (url) DO NOTHING RETURNING id",
                                          url,
                                          depth);

    if (doc_res.empty())
      return;

    int doc_id = doc_res[0][0].as<int>();

    for (const auto &[word, count] : word_counts) {

      pqxx::result word_res = tx.exec_params("INSERT INTO words (word) VALUES ($1) "
                                             "ON CONFLICT (word) DO NOTHING RETURNING id",
                                             word);

      int word_id;
      if (word_res.empty()) {

        word_res = tx.exec_params("SELECT id FROM words WHERE word = $1", word);
        word_id  = word_res[0][0].as<int>();
      } else {
        word_id = word_res[0][0].as<int>();
      }

      tx.exec_params("INSERT INTO document_word (document_id, word_id, count) "
                     "VALUES ($1, $2, $3) ON CONFLICT DO NOTHING",
                     doc_id,
                     word_id,
                     count);
    }

    tx.commit();
  } catch (const std::exception &e) {
    tx.abort();
    std::cerr << "Database error: " << e.what() << std::endl;
  }
}