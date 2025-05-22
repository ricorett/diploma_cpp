#include "database.h"

#include <iostream>

Database::Database(const std::string &connection_string)
    : conn{connection_string} {
  try {
    std::cout << "Connected to: " << conn.dbname() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    std::exit(1);
  }
}

int Database::insertDocument(const std::string &url, const std::string &content) {
  try {
    pqxx::work txn(conn);
    pqxx::result result = txn.exec(
        "INSERT INTO documents (url, content) VALUES (" +
        txn.quote(url) + ", " + txn.quote(content) +
        ") ON CONFLICT (url) DO NOTHING RETURNING id;"
    );

    int docId;
    if (!result.empty()) {
      docId = result[0][0].as<int>();
      std::cout << "[DEBUG] Document inserted with ID: " << docId << std::endl;
    } else {
      pqxx::result fallback = txn.exec("SELECT id FROM documents WHERE url = " + txn.quote(url) + ";");
      if (!fallback.empty()) {
        docId = fallback[0][0].as<int>();
        std::cout << "[DEBUG] Document already exists, ID: " << docId << std::endl;
      } else {
        std::cerr << "[ERROR] Failed to get ID for URL: " << url << std::endl;
        return -1;
      }
    }

    txn.commit();
    return docId;
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
    return -1;
  }
}

int Database::insertWord(const std::string &word) {
  try {
    pqxx::work txn(conn);
    auto result =
        txn.exec("INSERT INTO words (word) VALUES (" + txn.quote(word) +
                 ") ON CONFLICT (word) DO NOTHING RETURNING id;");
    txn.commit();
    return result.empty() ? getWordId(word) : result[0][0].as<int>();
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
    return -1;
  }
}

// void Database::insertWordsWithFrequency(int docId, const std::unordered_map<std::string, int>& wordFreq) {
//   try {
//     pqxx::work txn(conn);
//     for (const auto& [word, freq] : wordFreq) {
//       std::cout << "[DEBUG] Inserting word: " << word << ", freq: " << freq << std::endl;
//
//       auto result = txn.exec(
//           "INSERT INTO words (word) VALUES (" + txn.quote(word) + ") "
//           "ON CONFLICT (word) DO NOTHING RETURNING id;"
//       );
//
//       int wordId;
//       if (!result.empty()) {
//         wordId = result[0][0].as<int>();
//       } else {
//         auto selectRes = txn.exec("SELECT id FROM words WHERE word = " + txn.quote(word) + ";");
//         if (!selectRes.empty()) {
//           wordId = selectRes[0][0].as<int>();
//         } else {
//           std::cerr << "[ERROR] Word ID not found for: " << word << std::endl;
//           continue;
//         }
//       }
//
//       txn.exec(
//           "INSERT INTO inverted_index (doc_id, word_id, frequency) VALUES (" +
//           txn.quote(docId) + ", " + txn.quote(wordId) + ", " + txn.quote(freq) + ") "
//           "ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = EXCLUDED.frequency;"
//       );
//     }
//     txn.commit();
//   } catch (const std::exception& e) {
//     std::cerr << "Database error in insertWordsWithFrequency: " << e.what() << std::endl;
//   }
// }

void Database::insertWordsWithFrequency(int docId, const std::unordered_map<std::string, int>& wordFreq) {
  try {
    pqxx::work txn(conn);

    for (const auto& [word, freq] : wordFreq) {
      int wordId = -1;

      pqxx::result wordInsert = txn.exec(
          "INSERT INTO words (word) VALUES (" + txn.quote(word) + ") "
          "ON CONFLICT (word) DO NOTHING RETURNING id;"
      );

      if (!wordInsert.empty()) {
        wordId = wordInsert[0][0].as<int>();
        std::cout << "[DEBUG] Inserted word: " << word << " (new id: " << wordId << ")" << std::endl;
      } else {
        pqxx::result wordSelect = txn.exec(
            "SELECT id FROM words WHERE word = " + txn.quote(word) + ";"
        );

        if (!wordSelect.empty()) {
          wordId = wordSelect[0][0].as<int>();
          std::cout << "[DEBUG] Fetched word id: " << wordId << " for word: " << word << std::endl;
        } else {
          std::cerr << "[ERROR] Word not found: " << word << std::endl;
          continue;
        }
      }

      txn.exec(
          "INSERT INTO inverted_index (doc_id, word_id, frequency) VALUES (" +
          txn.quote(docId) + ", " + txn.quote(wordId) + ", " + txn.quote(freq) + ") "
          "ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = EXCLUDED.frequency;"
      );

      std::cout << "[DEBUG] Indexed: " << word << " -> docId: " << docId << ", freq: " << freq << std::endl;
    }

    txn.commit();
    std::cout << "[DEBUG] insertWordsWithFrequency committed." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] insertWordsWithFrequency: " << e.what() << std::endl;
  }
}



int Database::getWordId(const std::string &word) {
  try {
    pqxx::work txn(conn);
    auto result =
        txn.exec("SELECT id FROM words WHERE word = " + txn.quote(word) + ";");
    txn.commit();
    return result.empty() ? -1 : result[0][0].as<int>();
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
    return -1;
  }
}

void Database::insertIndex(int docId, int wordId, int frequency) {
  try {
    pqxx::work txn(conn);
    txn.exec(
        "INSERT INTO inverted_index (doc_id, word_id, frequency) VALUES (" +
        txn.quote(docId) + ", " + txn.quote(wordId) + ", " +
        txn.quote(frequency) +
        ") ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = "
        "EXCLUDED.frequency;");
    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
  }
}

std::vector<std::pair<std::string, int>> Database::searchDocuments(
    const std::vector<std::string> &queryWords) {
  std::vector<std::pair<std::string, int>> results;
  if (queryWords.empty()) return results;

  try {
    pqxx::work txn(conn);
    pqxx::result res = txn.exec(
        "SELECT d.url, SUM(idx.frequency) as relevance "
        "FROM inverted_index idx "
        "JOIN words w ON idx.word_id = w.id "
        "JOIN documents d ON idx.doc_id = d.id "
        "WHERE w.word = ANY(" +
        txn.quote(queryWords) +
        ") "
        "GROUP BY d.id "
        "ORDER BY relevance DESC "
        "LIMIT 10;");

    for (const auto &row : res) {
      results.emplace_back(row[0].as<std::string>(), row[1].as<int>());
    }
    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "Database error: " << e.what() << std::endl;
  }

  return results;
}

void Database::initializeTables() {
  try {
    pqxx::work txn(conn);

    txn.exec(
        "CREATE TABLE IF NOT EXISTS documents ("
        "id SERIAL PRIMARY KEY,"
        "url TEXT UNIQUE NOT NULL,"
        "content TEXT NOT NULL);");

    txn.exec(
        "CREATE TABLE IF NOT EXISTS words ("
        "id SERIAL PRIMARY KEY,"
        "word TEXT UNIQUE NOT NULL);");

    txn.exec(
        "CREATE TABLE IF NOT EXISTS index ("
        "doc_id INT REFERENCES documents(id) ON DELETE CASCADE,"
        "word_id INT REFERENCES words(id) ON DELETE CASCADE,"
        "frequency INT NOT NULL,"
        "PRIMARY KEY(doc_id, word_id));");

    txn.commit();
    std::cout << "Tables ensured to exist." << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Error creating tables: " << e.what() << std::endl;
    std::exit(1);
  }
}
