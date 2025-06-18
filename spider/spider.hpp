#pragma once
#include "../common/ini_parser.hpp"
#include "../database/database.hpp"
#include "http_utils.hpp"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>

class Spider {
public:
  Spider(IniParser &config, Database &db);
  void run();
  void stop();

private:
  IniParser                              &config;
  Database                               &db;
  std::queue<std::pair<std::string, int>> url_queue;
  std::mutex                              queue_mutex;
  std::condition_variable                 queue_cv;
  std::unordered_set<std::string>         visited_urls;
  std::mutex                              visited_mutex;
  std::atomic<int>                        active_tasks;
  std::atomic<bool>                       done;

  void add_url(const std::string &url, int depth);
  void worker();
  void process_url(const std::string &url, int depth);
};