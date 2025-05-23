#pragma once
#include <string>
#include <unordered_map>


class Config {
public:
    explicit Config(const std::string& path);

    std::string db_connection() const;
    std::string start_url() const;
    int spider_depth() const;
    std::string get(const std::string& key,
                   const std::string& default_val = "") const;

private:
    std::unordered_map<std::string, std::string> params_;
    void parse_ini(const std::string& path);
};

std::string load_connection_string(const std::string& config_path);
int load_server_port(const std::string& config_path);