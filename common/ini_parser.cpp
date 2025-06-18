#include "ini_parser.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

IniParser::IniParser(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open config file: " + filename);
  }

  std::string line;
  std::string currentSection;

  while (std::getline(file, line)) {
    trim(line);

    if (line.empty() || line[0] == ';' || line[0] == '#') {
      continue;
    }

    if (line[0] == '[' && line.back() == ']') {
      currentSection = line.substr(1, line.size() - 2);
      continue;
    }

    size_t pos = line.find('=');
    if (pos != std::string::npos) {
      std::string key   = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      trim(key);
      trim(value);

      sections[currentSection][key] = value;
    }
  }
}

void IniParser::trim(std::string &str) const {

  str.erase(0, str.find_first_not_of(" \t\r\n"));

  str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

std::string IniParser::getDbHost() const {
  return sections.at("database").at("host");
}

std::string IniParser::getDbPort() const {
  return sections.at("database").at("port");
}

std::string IniParser::getDbName() const {
  return sections.at("database").at("name");
}

std::string IniParser::getDbUser() const {
  return sections.at("database").at("user");
}

std::string IniParser::getDbPassword() const {
  return sections.at("database").at("password");
}

std::string IniParser::getStartUrl() const {
  return sections.at("spider").at("start_url");
}

int IniParser::getMaxDepth() const {
  return std::stoi(sections.at("spider").at("max_depth"));
}

int IniParser::getNumThreads() const {
  return std::stoi(sections.at("spider").at("num_threads"));
}

int IniParser::getServerPort() const {
  return std::stoi(sections.at("server").at("port"));
}