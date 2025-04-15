#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "http_utils.h"
#include "html_parser.h"
#include <functional>
#include "../database/database.h"

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;

// Убрана глобальная переменная db
// Database теперь создается внутри main()

void threadPoolWorker(Database& db) {  // Добавлен параметр db
    std::unique_lock<std::mutex> lock(mtx);
    while (!exitThreadPool || !tasks.empty()) {
        if (tasks.empty()) {
            cv.wait(lock);
        } else {
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();
            task();
            lock.lock();
        }
    }
}

void parseLink(const Link& link, int depth, Database& db) {
    try {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string html = getHtmlContent(link);
        if (html.empty()) {
            std::cout << "Failed to get HTML Content for: " << link.hostName + link.query << std::endl;
            return;
        }

        std::vector<std::string> words = extractWordsFromHtml(html);
        if (words.empty()) {
            std::cout << "No valid words extracted from: " << link.hostName + link.query << std::endl;
            return;
        }

        // Сохраняем документ в БД
        int docId = db.insertDocument(link.hostName + link.query, html);
        if (docId == -1) {
            std::cout << "Failed to insert document: " << link.hostName + link.query << std::endl;
            return;
        }

        // Подсчет частотности слов
        std::unordered_map<std::string, int> wordFrequency;
        for (const auto& word : words) {
            wordFrequency[word]++;
        }

        // Раскомментирован вызов
        db.insertWordsWithFrequency(docId, wordFrequency);

        std::cout << "Indexed document: " << link.hostName + link.query
                  << " (" << words.size() << " words, " << wordFrequency.size() << " unique)" << std::endl;

        // Добавление ссылок в очередь
        if (depth > 0) {
            std::vector<Link> links = {
                    {ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
                    {ProtocolType::HTTPS, "wikimediafoundation.org", "/"}
            };

            std::lock_guard<std::mutex> lock(mtx);
            for (auto& subLink : links) {
                tasks.push([subLink, depth, &db]() { parseLink(subLink, depth - 1, db); });
            }
            cv.notify_one();
        }
    } catch (const std::exception& e) {
        std::cout << "Error parsing link " << link.hostName + link.query << ": " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Создаем объект Database внутри main()
        Database db;

        int numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threadPool;

        // Передаем db в threadPoolWorker
        for (int i = 0; i < numThreads; ++i) {
            threadPool.emplace_back([&db]() { threadPoolWorker(db); });
        }

        Link link{ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Main_Page"};

        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.push([&link, &db]() { parseLink(link, 1, db); });
            cv.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        {
            std::lock_guard<std::mutex> lock(mtx);
            exitThreadPool = true;
            cv.notify_all();
        }

        for (auto& t : threadPool) {
            t.join();
        }
    } catch (const std::exception& e) {
        std::cout << "Main error: " << e.what() << std::endl;
    }
    return 0;
}