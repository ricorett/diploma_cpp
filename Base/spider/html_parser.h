#pragma once
#include <gumbo.h>

#include <cctype>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>


#include "../database/database.h"
#include "http_utils.h"
#include "link.h"

void processStartUrl(const std::string& startUrl, int depth, Database& db);
std::vector<std::string> extractWordsFromHtml(const std::string& html);
void extractText(GumboNode* node, std::string& output);
void cleanText(std::string& text);
void parseLink(const Link& link, int depth, Database& db);
Link parseLinkFromUrl(const std::string& url);
std::vector<std::string> extractLinksFromHtml(const std::string& html);
std::string generateHtmlResults(
    const std::vector<std::pair<std::string, int>>& results);