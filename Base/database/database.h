#pragma once
#include <pqxx/pqxx>
#include <string>
#include <boost/algorithm/string/join.hpp>
class Database {
public:
    explicit Database(const std::string& conn_str);

    void initialize_tables();
    int insert_document(const std::string& url, const std::string& content);
    void insert_words_frequency(int doc_id,
                                    const std::unordered_map<std::string, int>& freq);
    std::vector<std::pair<std::string, int>> search(const std::vector<std::string>& terms);

private:
    pqxx::connection conn_;

    int get_word_id(pqxx::work& txn, const std::string& word);
};

std::string make_sql_array(const std::vector<std::string>& terms, pqxx::transaction_base& txn);