#include "html_parser.h"


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
        if (word.length() >= 3 && word.length() <= 32) {
            words.push_back(word);
        }
    }
    return words;
}