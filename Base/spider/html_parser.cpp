#include "html_parser.h"
#include <gumbo.h>
#include <algorithm>
#include <functional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include "link.h"

using namespace std;

// Вспомогательная функция для извлечения текста из HTML-узлов
static void extract_text_from_node(GumboNode* node, string& output) {
    if (node->type == GUMBO_NODE_TEXT) {
        output += node->v.text.text;
        output += " ";
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        GumboVector* children = &node->v.element.children;
        for (unsigned i = 0; i < children->length; ++i) {
            extract_text_from_node(static_cast<GumboNode*>(children->data[i]), output);
        }
    }
}

// Извлечение слов из HTML-контента
vector<string> extract_words(const string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    string raw_text;
    extract_text_from_node(output->root, raw_text);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    // Очистка и нормализация текста
    transform(raw_text.begin(), raw_text.end(), raw_text.begin(), ::tolower);
    raw_text = regex_replace(raw_text, regex(R"([^a-zа-яё0-9\- ])"), " ");

    // Разделение на слова
    vector<string> words;
    istringstream iss(raw_text);
    string word;
    while (iss >> word) {
        if (word.length() >= 2) {
            words.push_back(std::move(word));
        }
    }
    return words;
}

// Извлечение ссылок из HTML
vector<Link> extract_links(const string& html) {
    vector<Link> links;
    GumboOutput* output = gumbo_parse(html.c_str());

    function<void(GumboNode*)> find_links = [&](GumboNode* node) {
        if (node->type != GUMBO_NODE_ELEMENT) return;

        if (node->v.element.tag == GUMBO_TAG_A) {
            GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (href && href->value) {
                try {
                    links.push_back(parse_url(href->value));
                } catch(const invalid_argument&) {
                    // Пропускаем невалидные URL
                }
            }
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned i = 0; i < children->length; ++i) {
            find_links(static_cast<GumboNode*>(children->data[i]));
        }
    };

    find_links(output->root);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return links;
}

Link parse_url(const string& url) {
    regex url_regex(R"((https?)://([^/]+)(.*))", regex::icase);
    smatch match;

    if (!regex_match(url, match, url_regex) || match.size() < 4) {
        throw invalid_argument("Invalid URL: " + url);
    }

    Link link;
    link.protocol = (match[1] == "https") ? ProtocolType::HTTPS : ProtocolType::HTTP;
    link.hostName = match[2];
    link.query = match[3].str();

    // Нормализация query
    if (link.query.empty()) {
        link.query = "/";
    } else if (link.query[0] != '/') {
        link.query = "/" + link.query;
    }

    return link;
}