#pragma once
#include <string>
#include <vector>
#include <gumbo.h>
#include <regex>
#include <unordered_set>
#include <sstream>
#include <cctype>

std::vector<std::string> extractWordsFromHtml(const std::string& html);
void extractText(GumboNode* node, std::string& output);
void cleanText(std::string& text);