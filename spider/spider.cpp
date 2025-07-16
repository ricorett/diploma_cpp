#include "spider.hpp"
#include "../html_parser/html_parser.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
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

  // Normalize URL
  std::string normalized_url;
  try {
    if (auto parsed = boost::urls::parse_uri(url)) {
      if (parsed) {
        boost::urls::url normalized_url(*parsed);
        normalized_url.normalize();
        // Use normalized_url instead of parsed
      }
      normalized_url = parsed->buffer();

      // Fix double https
      size_t dbl_proto = normalized_url.find("https:https://");
      if (dbl_proto != std::string::npos) {
        normalized_url.replace(dbl_proto, 8, "");
      }
    } else {
      normalized_url = url;
    }
  } catch (...) {
    normalized_url = url;
  }

  // Remove fragments
  size_t fragment_pos = normalized_url.find('#');
  if (fragment_pos != std::string::npos) {
    normalized_url = normalized_url.substr(0, fragment_pos);
  }

  {
    std::lock_guard<std::mutex> lock(visited_mutex);
    if (visited_urls.find(normalized_url) != visited_urls.end()) {
      return;
    }
    visited_urls.insert(normalized_url);
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    url_queue.push({normalized_url, depth});
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

    std::string final_url, html;
    try {
      auto result = download(url);
      final_url   = result.first;
      html        = result.second;
    } catch (const std::exception &e) {
      std::cerr << "Download error: " << e.what() << " for " << url << std::endl;
      return;
    }

    if (html.empty()) {
      std::cerr << "Empty content: " << final_url << std::endl;
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

    db.addDocument(final_url, depth, word_counts);

    if (depth < config.getMaxDepth()) {
      std::vector<std::string> links = HtmlParser::extractLinks(html, final_url);
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