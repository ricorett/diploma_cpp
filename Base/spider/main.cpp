#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "../config/config_utils.h"
#include "../database/database.h"
#include "html_parser.h"
#include "http_utils.h"

std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;

void threadPoolWorker(Database& db) {
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
      std::cout << "Failed to get HTML Content for: "
                << link.hostName + link.query << std::endl;
      return;
    }

    std::vector<std::string> words = extractWordsFromHtml(html);
    if (words.empty()) {
      std::cout << "No valid words extracted from: "
                << link.hostName + link.query << std::endl;
      return;
    }

    int docId = db.insertDocument(link.hostName + link.query, html);
    if (docId == -1) {
      std::cout << "Failed to insert document: " << link.hostName + link.query
                << std::endl;
      return;
    }

    std::unordered_map<std::string, int> wordFrequency;
    for (const auto& word : words) {
      wordFrequency[word]++;
    }

    db.insertWordsWithFrequency(docId, wordFrequency);

    std::cout << "Indexed document: " << link.hostName + link.query << " ("
              << words.size() << " words, " << wordFrequency.size()
              << " unique)" << std::endl;

    // Добавление ссылок в очередь
    if (depth > 0) {
      std::vector<Link> links = {
          {ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
          {ProtocolType::HTTPS, "wikimediafoundation.org", "/"}};

      std::lock_guard<std::mutex> lock(mtx);
      for (auto& subLink : links) {
        tasks.emplace(
            [subLink, depth, &db]() { parseLink(subLink, depth - 1, db); });
      }
      cv.notify_one();
    }
  } catch (const std::exception& e) {
    std::cout << "Error parsing link " << link.hostName + link.query << ": "
              << e.what() << std::endl;
  }
}

void processStartUrl(const std::string& startUrl, int depth, Database& db) {
  try {
    Link link = parseLinkFromUrl(startUrl);
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push([link, depth, &db]() { parseLink(link, depth, db); });
    cv.notify_one();
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Invalid start URL: " << e.what() << std::endl;
  }
}

int main() {
  try {
    Database db(loadConnectionString("../config/config.ini"));
    db.initializeTables();

    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threadPool;
    for (int i = 0; i < numThreads; ++i) {
      threadPool.emplace_back([&db]() { threadPoolWorker(db); });
    }

    std::string startUrl = loadStartUrl("../config/config.ini");
    int depth = loadSpiderDepth("../config/config.ini");
    processStartUrl(startUrl, depth, db);

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
    std::cerr << "Main error: " << e.what() << std::endl;
  }
  return 0;
}
