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

int Database::insertDocument(const std::string &url,
                             const std::string &content) {
  try {
    pqxx::work txn(conn);
    auto result = txn.exec("INSERT INTO documents (url, content) VALUES (" +
                           txn.quote(url) + ", " + txn.quote(content) +
                           ") RETURNING id;");
    txn.commit();
    return result[0][0].as<int>();
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

void Database::insertWordsWithFrequency(
    int docId, const std::unordered_map<std::string, int> &wordFreq) {
  try {
    pqxx::work txn(conn);

    for (const auto &[word, freq] : wordFreq) {
      // Вставка слова, если оно ещё не существует
      pqxx::result wordRes = txn.exec_params(
          "INSERT INTO words (word) VALUES ($1) "
          "ON CONFLICT (word) DO NOTHING RETURNING id;",
          word);

      int wordId;
      if (!wordRes.empty()) {
        wordId = wordRes[0][0].as<int>();
      } else {
        // Если слово уже есть, получаем его id
        wordRes =
            txn.exec_params("SELECT id FROM words WHERE word = $1;", word);
        wordId = wordRes[0][0].as<int>();
      }

      // Добавление или обновление частоты слова для документа
      txn.exec_params(
          "INSERT INTO index (doc_id, word_id, frequency) VALUES ($1, $2, $3) "
          "ON CONFLICT (doc_id, word_id) DO UPDATE SET frequency = "
          "EXCLUDED.frequency;",
          docId, wordId, freq);
    }

    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "insertWordsWithFrequency error: " << e.what() << std::endl;
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
