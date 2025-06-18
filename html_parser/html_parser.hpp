#pragma once
#include <string>
#include <vector>

class HtmlParser {
public:
  static std::string              cleanHtml(const std::string &html);
  static std::vector<std::string> extractLinks(const std::string &html, const std::string &base_url);
};