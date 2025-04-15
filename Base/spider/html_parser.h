#pragma once
#include <string>
#include <vector>
#include <gumbo.h>
#include <regex>
#include <unordered_set>
#include <sstream>
#include <cctype>
#include "../database/database.h"
#include "link.h"
#include "http_utils.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

std::vector<std::string> extractWordsFromHtml(const std::string& html);
void extractText(GumboNode* node, std::string& output);
void cleanText(std::string& text);
void parseLink(const Link& link, int depth, Database& db);