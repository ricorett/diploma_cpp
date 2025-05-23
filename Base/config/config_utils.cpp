#include "config_utils.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

Config::Config(const std::string& path) {
    parse_ini(path);
}

void Config::parse_ini(const std::string& path) {
    std::ifstream file(path);
    std::string line, section;

    while(std::getline(file, line)) {
        if(line.empty() || line[0] == ';') continue;

        if(line[0] == '[') {
            section = line.substr(1, line.find(']') - 1);
            continue;
        }

        size_t eq = line.find('=');
        if(eq != std::string::npos) {
            std::string key = section + "." + line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            params_[key] = value;
        }
    }
}


std::string Config::start_url() const {
    return params_.at("spider.start_url");
}

int Config::spider_depth() const {
    return std::stoi(params_.at("spider.depth"));
}

std::string load_connection_string(const std::string& config_path) {
    Config config(config_path);
    return "postgresql://"
           + config.get("database.user") + ":"
           + config.get("database.password") + "@"
           + config.get("database.host") + ":"
           + config.get("database.port") + "/"
           + config.get("database.dbname");
}

std::string Config::get(const std::string& key,
                       const std::string& default_val) const {
    auto it = params_.find(key);
    return (it != params_.end()) ? it->second : default_val;
}


int load_server_port(const std::string& config_path) {
    Config config(config_path);
    try {
        int port = std::stoi(config.get("server.port", "8080"));
        if(port < 1 || port > 65535) {
            throw std::out_of_range("Port must be between 1 and 65535");
        }
        return port;
    } catch(const std::exception& e) {
        std::cerr << "Invalid port in config: " << e.what()
                  << ". Using default 8080\n";
        return 8080;
    }
}

std::string Config::db_connection() const {
    return "postgresql://"
        + get("database.user") + ":"
        + get("database.password") + "@"
        + get("database.host") + ":"
        + get("database.port") + "/"
        + get("database.dbname");
}