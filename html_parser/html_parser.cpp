#include "html_parser.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/pct_string_view.hpp>
#include <boost/url/url.hpp>
#include <iostream>

std::string HtmlParser::cleanHtml(const std::string &html) {
  static const boost::regex tags_re(R"(<[^>]*>)");
  std::string               text = boost::regex_replace(html, tags_re, " ");

  static const boost::regex entities_re(R"(&(?:amp|lt|gt|quot|#39);)");
  text = boost::regex_replace(text, entities_re, " ");

  static const boost::regex punct_re(R"([^\w\s])");
  text = boost::regex_replace(text, punct_re, " ");

  boost::algorithm::to_lower(text);


  static const boost::regex space_re(R"(\s+)");
  text = boost::regex_replace(text, space_re, " ");

  boost::trim(text);

  return text;
}

std::vector<std::string> HtmlParser::extractLinks(const std::string &html, const std::string &base_url) {
  std::vector<std::string>  links;
  static const boost::regex link_re(R"lit(<a\s+[^>]*?href\s*=\s*["']([^"']+)["'])lit");
  boost::sregex_iterator    it(html.begin(), html.end(), link_re), end;


  auto base = boost::urls::parse_uri(base_url);
  if (!base) {
    std::cerr << "Invalid base URL: " << base_url << std::endl;
    return links;
  }
  boost::urls::url base_url_obj = *base;

  while (it != end) {
    std::string link = (*it)[1].str();
    ++it;


    if (boost::algorithm::starts_with(link, "javascript:") || boost::algorithm::starts_with(link, "mailto:") ||
        boost::algorithm::starts_with(link, "tel:") || boost::algorithm::starts_with(link, "#")) {
      continue;
        }

    try {
      boost::urls::url_view link_view(link);


      boost::urls::url resolved_url = base_url_obj;


      auto res = resolved_url.resolve(link_view);
      if (!res) {
        std::cerr << "Failed to resolve URL: " << link << " Error: " << res.error().message() << std::endl;
        continue;
      }

      resolved_url.remove_fragment();
      resolved_url.normalize();

      links.push_back(resolved_url.buffer());
    }
    catch (const std::exception &e) {
      std::cerr << "Error processing link: " << link << " - " << e.what() << std::endl;
    }
  }

  return links;
}