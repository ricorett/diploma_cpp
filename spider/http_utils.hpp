#pragma once

#include <string>
#include <utility>

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
};

std::pair<std::string, std::string> download(const std::string &url);
ParsedUrl parse_url(const std::string &url);