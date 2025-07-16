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
  pqxx::work tx(*conn);

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
void Database::addDocument(const std::string &url, int depth,
                           const std::map<std::string, int> &word_counts) {
  std::lock_guard<std::mutex> lock(db_mutex);
  pqxx::work tx(*conn);

  try {
    // Insert document
    pqxx::result doc_res =
        tx.exec_params("INSERT INTO documents (url, depth) VALUES ($1, $2) "
                       "ON CONFLICT (url) DO NOTHING RETURNING id",
                       url, depth);

    if (doc_res.empty()) {
      tx.commit(); // Commit empty transaction
      return;
    }

    int doc_id = doc_res[0][0].as<int>();
    int savepoint_counter = 0;

    for (const auto &[word, count] : word_counts) {
      std::string savepoint_name = "sp_" + std::to_string(savepoint_counter++);

      try {
        // Create savepoint
        tx.exec("SAVEPOINT " + savepoint_name);

        // Insert word
        pqxx::result word_res =
            tx.exec_params("INSERT INTO words (word) VALUES ($1) "
                           "ON CONFLICT (word) DO NOTHING RETURNING id",
                           word);

        int word_id;
        if (word_res.empty()) {
          word_res =
              tx.exec_params("SELECT id FROM words WHERE word = $1", word);
          if (word_res.empty()) {
            throw std::runtime_error("Word not found after conflict: " + word);
          }
          word_id = word_res[0][0].as<int>();
        } else {
          word_id = word_res[0][0].as<int>();
        }

        // Insert document-word relationship
        tx.exec_params(
            "INSERT INTO document_word (document_id, word_id, count) "
            "VALUES ($1, $2, $3) ON CONFLICT DO NOTHING",
            doc_id, word_id, count);

        // Release savepoint
        tx.exec("RELEASE SAVEPOINT " + savepoint_name);
      } catch (const std::exception &e) {
        // Rollback to savepoint on error
        try {
          tx.exec("ROLLBACK TO SAVEPOINT " + savepoint_name);
        } catch (const std::exception &inner_e) {
          // Critical error - abort entire transaction
          std::cerr << "CRITICAL savepoint error: " << inner_e.what()
                    << std::endl;
          throw;
        }
        std::cerr << "Skipped word '" << word << "': " << e.what() << std::endl;
      }
    }

    tx.commit();
  } catch (const std::exception &e) {
    tx.abort();
    std::cerr << "Database error: " << e.what() << std::endl;
  }
}