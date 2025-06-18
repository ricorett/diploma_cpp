#include "../common/ini_parser.hpp"
#include "../database/database.hpp"
#include "spider.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
  try {
    if (argc < 2) {
      throw std::runtime_error("Need path to config");
    }

    IniParser config(argv[1]);

    std::string conn_str = "dbname=" + config.getDbName() +
                           " "
                           "user=" +
                           config.getDbUser() +
                           " "
                           "password=" +
                           config.getDbPassword() +
                           " "
                           "host=" +
                           config.getDbHost() +
                           " "
                           "port=" +
                           config.getDbPort();

    Database db(conn_str);
    db.initialize();

    Spider spider(config, db);
    spider.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}