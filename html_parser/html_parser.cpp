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

  // Remove extra spaces
  static const boost::regex space_re(R"(\s+)");
  text = boost::regex_replace(text, space_re, " ");

  boost::trim(text);

  return text;
}

std::vector<std::string> HtmlParser::extractLinks(const std::string &html, const std::string &base_url) {
  std::vector<std::string>  links;
  static const boost::regex link_re(R"lit(<a\s+[^>]*?href\s*=\s*["']([^"']+)["'])lit");
  boost::sregex_iterator    it(html.begin(), html.end(), link_re), end;

  // Parse base URL
  auto base = boost::urls::parse_uri(base_url);
  if (!base) {
    std::cerr << "Invalid base URL: " << base_url << std::endl;
    return links;
  }
  boost::urls::url base_url_obj = *base;

  while (it != end) {
    std::string link = (*it)[1].str();
    ++it;

    // Skip unwanted schemes
    if (boost::algorithm::starts_with(link, "javascript:") || boost::algorithm::starts_with(link, "mailto:") ||
        boost::algorithm::starts_with(link, "tel:") || boost::algorithm::starts_with(link, "#")) {
      continue;
    }

    try {
      // Handle relative URLs
      if (auto parsed = boost::urls::parse_uri(link); parsed && parsed->scheme().empty()) {
        boost::urls::url resolved = base_url_obj;

        if (!link.empty() && link[0] == '/') {
          // Absolute path
          resolved.set_path(link);
        } else {
          // Relative path
          std::string current_path = resolved.path();
          if (current_path.empty()) {
            current_path = "/";
          }

          // Find last slash
          size_t last_slash = current_path.find_last_of('/');
          if (last_slash == std::string::npos) {
            current_path = "/" + link;
          } else {
            current_path = current_path.substr(0, last_slash + 1) + link;
          }
          resolved.set_path(current_path);
        }

        // Remove fragment and normalize
        resolved.remove_fragment();
        resolved.normalize();

        links.push_back(resolved.buffer());
      } else {
        // Absolute URL
        if (auto abs_url = boost::urls::parse_uri(link)) {
          if (abs_url) {
            boost::urls::url modifiable_url(*abs_url);
            modifiable_url.remove_fragment();
            modifiable_url.normalize();
            // Use modifiable_url
          }

          // Fix double https
          std::string fixed_url = abs_url->buffer();
          size_t      dbl_proto = fixed_url.find("https://https://");
          if (dbl_proto != std::string::npos) {
            fixed_url.replace(dbl_proto, 8, "");
          }

          links.push_back(fixed_url);
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "Error processing link: " << link << " - " << e.what() << std::endl;
    }
  }

  return links;
}