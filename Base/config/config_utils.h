#pragma once
#include <string>

std::string loadConnectionString(const std::string& configPath);
std::string loadStartUrl(const std::string& configPath);
int loadSpiderDepth(const std::string& configPath);
int loadServerPort(const std::string& configPath);