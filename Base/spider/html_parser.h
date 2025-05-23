#pragma once
#include <string>
#include <vector>
#include "link.h"


std::vector<std::string> extract_words(const std::string& html);
std::vector<Link> extract_links(const std::string& html);
Link parse_url(const std::string& url);