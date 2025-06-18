#pragma once
#include <map>
#include <string>

class IniParser {
public:
  IniParser(const std::string &filename);

  std::string getDbHost() const;
  std::string getDbPort() const;
  std::string getDbName() const;
  std::string getDbUser() const;
  std::string getDbPassword() const;

  std::string getStartUrl() const;
  int         getMaxDepth() const;
  int         getNumThreads() const;

  int getServerPort() const;

private:
  std::map<std::string, std::map<std::string, std::string>> sections;

  void trim(std::string &str) const;
};