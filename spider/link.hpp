#pragma once
#include <string>

struct ParsedUrl {
  std::string host;
  std::string port = "80";
  std::string path = "/";
};

ParsedUrl parse_url(const std::string &url);