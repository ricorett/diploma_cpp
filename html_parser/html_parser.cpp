#include "html_parser.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>

std::string HtmlParser::cleanHtml(const std::string &html) {

  static const boost::regex tags_re(R"(<[^>]*>)");
  std::string               text = boost::regex_replace(html, tags_re, " ");

  static const boost::regex entities_re(R"(&(?:amp|lt|gt|quot|#39);)");
  text = boost::regex_replace(text, entities_re, " ");

  static const boost::regex punct_re(R"([^\w\s])");
  text = boost::regex_replace(text, punct_re, " ");

  boost::algorithm::to_lower(text);

  boost::trim(text);

  return text;
}

std::vector<std::string> HtmlParser::extractLinks(const std::string &html, const std::string &base_url) {
  std::vector<std::string>  links;
  static const boost::regex link_re(R"(<a\s+[^>]*?href\s*=\s*["']([^"']+)["'])");
  boost::sregex_iterator    it(html.begin(), html.end(), link_re), end;

  while (it != end) {
    std::string link = (*it)[1].str();
    ++it;

    if (boost::algorithm::starts_with(link, "javascript:") || boost::algorithm::starts_with(link, "mailto:") ||
        boost::algorithm::starts_with(link, "tel:") || boost::algorithm::starts_with(link, "#")) {
      continue;
    }

    if (boost::algorithm::starts_with(link, "/")) {

      link = "https://" + base_url + link;
    } else if (!boost::algorithm::starts_with(link, "http")) {
      continue;
    }

    size_t fragment_pos = link.find('#');
    if (fragment_pos != std::string::npos) {
      link = link.substr(0, fragment_pos);
    }

    links.push_back(link);
  }

  return links;
}