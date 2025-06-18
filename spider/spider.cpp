#include "spider.hpp"
#include "../html_parser/html_parser.hpp"
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <iostream>
#include <thread>

Spider::Spider(IniParser &config, Database &db)
    : config(config)
    , db(db)
    , active_tasks(0)
    , done(false) {
}

void Spider::run() {
  add_url(config.getStartUrl(), 0);

  std::vector<std::thread> threads;
  for (int i = 0; i < config.getNumThreads(); ++i) {
    threads.emplace_back(&Spider::worker, this);
  }

  for (auto &t : threads) {
    t.join();
  }

  std::cout << "Crawling completed. Total pages: " << visited_urls.size() << std::endl;
}

void Spider::add_url(const std::string &url, int depth) {
  if (done)
    return;
  if (depth > config.getMaxDepth())
    return;

  {
    std::lock_guard<std::mutex> lock(visited_mutex);
    if (visited_urls.find(url) != visited_urls.end()) {
      return;
    }
    visited_urls.insert(url);
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    url_queue.push({url, depth});
  }
  queue_cv.notify_one();
}

void Spider::worker() {
  while (!done) {
    std::pair<std::string, int> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      queue_cv.wait(lock, [this] { return !url_queue.empty() || done; });

      if (done)
        break;

      task = url_queue.front();
      url_queue.pop();
    }

    active_tasks++;
    process_url(task.first, task.second);
    active_tasks--;
  }
}

void Spider::process_url(const std::string &url, int depth) {
  try {
    std::cout << "Processing: " << url << " (depth: " << depth << ")" << std::endl;

    std::string html = download(url);
    if (html.empty()) {
      std::cerr << "Empty content: " << url << std::endl;
      return;
    }

    std::string text = HtmlParser::cleanHtml(html);

    std::map<std::string, int> word_counts;
    std::vector<std::string>   words;
    boost::split(words, text, boost::is_any_of(" \t\r\n"), boost::token_compress_on);

    for (const auto &word : words) {
      if (word.size() >= 3 && word.size() <= 32) {
        word_counts[word]++;
      }
    }

    db.addDocument(url, depth, word_counts);

    if (depth < config.getMaxDepth()) {
      std::vector<std::string> links = HtmlParser::extractLinks(html, url);
      for (const auto &link : links) {
        add_url(link, depth + 1);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error processing " << url << ": " << e.what() << std::endl;
  }
}

void Spider::stop() {
  done = true;
  queue_cv.notify_all();
}