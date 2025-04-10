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

//std::string dbName = "search_db";
//std::string dbUser = "postgres";
//std::string dbPassword = "password";
//std::string dbHost = "localhost";
//int dbPort = 5432;

Database db("/Users/lana/Desktop/work/netology/diploma/Base/database/config.ini");

void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}

void parseLink(const Link& link, int depth) {
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

        // Сохраняем слова в БД
        db.insertWordsWithFrequency(docId, wordFrequency);

        std::cout << "Indexed document: " << link.hostName + link.query
                  << " (" << words.size() << " words, " << wordFrequency.size() << " unique)" << std::endl;

        // Собираем ссылки для дальнейшего обхода (пока заглушка)
        std::vector<Link> links = {
                {ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
                {ProtocolType::HTTPS, "wikimediafoundation.org", "/"}
        };

        // Добавляем ссылки в очередь
        if (depth > 0) {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& subLink : links) {
                tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
            }
            cv.notify_one();
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error parsing link " << link.hostName + link.query << ": " << e.what() << std::endl;
    }
}



int main()
{
	try {
		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		Link link{ ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Main_Page" };


		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link]() { parseLink(link, 1); });
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
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
