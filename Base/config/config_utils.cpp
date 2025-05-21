#include "config_utils.h"

#include <fmt/format.h>

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>

std::string loadConnectionString(const std::string& configPath) {
  namespace fs = std::filesystem;
  using boost::property_tree::ptree;

  if (!fs::exists(configPath)) {
    throw std::runtime_error("Config file not found: " + configPath);
  }

  ptree pt;
  boost::property_tree::ini_parser::read_ini(configPath, pt);

  auto dbname = pt.get<std::string>("database.dbname");
  auto user = pt.get<std::string>("database.user");
  auto password = pt.get<std::string>("database.password");
  auto host = pt.get<std::string>("database.host");
  int port = pt.get<int>("database.port");

  return fmt::format("dbname={} user={} password={} host={} port={}", dbname,
                     user, password, host, port);
}

std::string loadStartUrl(const std::string& configPath) {
  boost::property_tree::ptree pt;
  boost::property_tree::ini_parser::read_ini(configPath, pt);
  return pt.get<std::string>("spider.start_url");
}

int loadSpiderDepth(const std::string& configPath) {
  boost::property_tree::ptree pt;
  boost::property_tree::ini_parser::read_ini(configPath, pt);
  return pt.get<int>("spider.depth");
}

int loadServerPort(const std::string& configPath) {
  boost::property_tree::ptree pt;
  boost::property_tree::ini_parser::read_ini(configPath, pt);
  return pt.get<int>("server.port");
}