#include "html_parser.h"

// extern std::mutex mtx;
// extern std::condition_variable cv;
// extern std::queue<std::function<void()>> tasks;


void cleanText(std::string& text) {
  text = std::regex_replace(text, std::regex(R"([^a-zA-Zа-яА-ЯёЁ\s])"), " ");
  std::transform(text.begin(), text.end(), text.begin(), ::tolower);
}

void extractText(GumboNode* node, std::string& output) {
  if (node->type == GUMBO_NODE_TEXT) {
    output.append(node->v.text.text);
    output.append(" ");
  } else if (node->type == GUMBO_NODE_ELEMENT) {
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      extractText(static_cast<GumboNode*>(children->data[i]), output);
    }
  }
}

std::vector<std::string> extractWordsFromHtml(const std::string& html) {
  GumboOutput* output = gumbo_parse(html.c_str());
  std::string rawText;
  extractText(output->root, rawText);
  gumbo_destroy_output(&kGumboDefaultOptions, output);

  cleanText(rawText);

  std::istringstream iss(rawText);
  std::string word;
  std::vector<std::string> words;
  while (iss >> word) {
    word.erase(std::remove_if(word.begin(), word.end(),
                                  [](char c) { return std::ispunct(c); }),
                   word.end());

    if (word.length() >= 3 && word.length() <= 32) {
      boost::algorithm::to_lower(word);
      words.push_back(word);
    }
  }
  std::cerr << "[DEBUG] Extracted " << words.size() << " words." << std::endl;
  return words;
}

Link parseLinkFromUrl(const std::string& url) {
  std::regex urlRegex(R"((http|https)://([^/]+)(/.*)?)");
  std::smatch matches;
  if (std::regex_match(url, matches, urlRegex)) {
    ProtocolType proto = matches[1] == "https" ? ProtocolType::HTTPS : ProtocolType::HTTP;
    std::string host = matches[2];
    std::string path = matches[3].matched ? matches[3].str() : "/";
    return {proto, host, path};
  }
  throw std::invalid_argument("Invalid URL format: " + url);
}

std::vector<std::string> extractLinksFromHtml(const std::string& html) {
  std::vector<std::string> links;
  GumboOutput* output = gumbo_parse(html.c_str());

  std::function<void(GumboNode*)> searchForLinks = [&](GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboElement* element = &node->v.element;
    if (element->tag == GUMBO_TAG_A) {
      GumboAttribute* hrefAttr =
          gumbo_get_attribute(&element->attributes, "href");
      if (hrefAttr && hrefAttr->value) {
        links.emplace_back(hrefAttr->value);
      }
    }

    GumboVector* children = &element->children;
    for (unsigned int i = 0; i < children->length; ++i) {
      searchForLinks(static_cast<GumboNode*>(children->data[i]));
    }
  };

  searchForLinks(output->root);
  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return links;
}

std::string generateHtmlResults(
    const std::vector<std::pair<std::string, int>>& results) {
  std::ostringstream html;
  html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
       << "<meta charset=\"UTF-8\">\n"
       << "<title>Search Results</title>\n"
       << "</head>\n<body>\n"
       << "<h1>Search Results</h1>\n";

  if (results.empty()) {
    html << "<p>No results found.</p>\n";
  } else {
    html << "<ul>\n";
    for (const auto& [url, relevance] : results) {
      html << "<li><a href=\"" << url << "\">" << url
           << "</a> (Relevance: " << relevance << ")</li>\n";
    }
    html << "</ul>\n";
  }

  html << "</body>\n</html>";
  return html.str();
}

