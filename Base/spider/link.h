#pragma once
#include <string>
#include <unordered_set>
// #include <regex>
// #include <stdexcept>

enum class ProtocolType { HTTP = 0, HTTPS = 1 };

struct Link {
  ProtocolType protocol;
  std::string hostName;
  std::string query;

  bool operator==(const Link& l) const {
    return protocol == l.protocol && hostName == l.hostName && query == l.query;
  }
};

// Link parseUrl(const std::string& url) {
//     std::regex urlRegex(R"((http|https)://([^/]+)(/.*)?)");
//     std::smatch matches;
//
//     if (std::regex_match(url, matches, urlRegex)) {
//         ProtocolType protocol = matches[1] == "https" ? ProtocolType::HTTPS :
//         ProtocolType::HTTP; std::string host = matches[2]; std::string path =
//         matches[3].matched ? matches[3] : "/";
//
//         return Link{protocol, host, path};
//     }
//
//     throw std::invalid_argument("Invalid URL format: " + url);
// }