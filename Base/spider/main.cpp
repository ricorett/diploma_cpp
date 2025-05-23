#include "html_parser.h"
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <unordered_set>
#include "../database/database.h"
#include "../config/config_utils.h"
#include "http_utils.h"
#include <boost/url.hpp>
#include <chrono>

class Spider {
public:
    Spider(Database& db, const std::string& start_url, int depth)
           : db_(db), start_url_(start_url), max_depth_(depth) {}

    ~Spider() {
        // Деструктор для ожидания завершения всех потоков
        for (auto& t : workers_) {
            if (t.joinable()) t.join();
        }
    }
    void run() {
        crawl(start_url_, max_depth_);
    }

private:
    Database& db_;
    std::string start_url_;
    int max_depth_;
    std::unordered_set<std::string> visited_;
    std::mutex mutex_;

    std::vector<std::thread> workers_;


    std::string resolve_url(const std::string& base, const std::string& path) {
        boost::urls::url base_url(base);
        auto result = base_url.resolve(path);
        return result.buffer();
    }

    void crawl(const std::string& url, int depth) {
        if (depth < 0) return;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!visited_.insert(url).second) return;
        }

        // Добавлена задержка между запросами
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string html = fetch_html(url);
        if (html.empty()) return;

        // Добавлено логирование
        std::cout << "Processing: " << url << " (depth: " << depth << ")\n";

        int doc_id = db_.insert_document(url, html);
        if (doc_id == -1) return;

        auto words = extract_words(html);
        std::unordered_map<std::string, int> word_freq;
        for (const auto& word : words) word_freq[word]++;
        db_.insert_words_frequency(doc_id, word_freq);

        if (depth > 0) {
            for (const auto& link : extract_links(html)) {
                // Исправлено формирование URL
                std::string full_url = resolve_url(url, link.query);

                workers_.emplace_back([this, full_url, depth] {
                    crawl(full_url, depth - 1);
                });
            }
        }
    }


    static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }

    std::string fetch_html(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "CURL init failed for: " << url << "\n";
            return "";
        }

        std::string response;

        // Добавлены параметры CURL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MySpider/1.0");

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL failed (" << curl_easy_strerror(res)
                     << "): " << url << "\n";
        }

        curl_easy_cleanup(curl);
        return (res == CURLE_OK) ? response : "";
    }
};

int main() {
    try {
        Config config("../config/config.ini");
        Database db(config.db_connection());
        db.initialize_tables();

        {
            Spider crawler(db, config.start_url(), config.spider_depth());
            crawler.run();

        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}