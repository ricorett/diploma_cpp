#include "http_connection.h"
#include "../config/config_utils.h"
#include <boost/asio.hpp>
#include <iostream>

int main() {
  try {
    // Инициализация базы данных
    Database db(load_connection_string("../config/config.ini"));
    db.initialize_tables();

    // Настройка сервера
    net::io_context ioc;
    const unsigned short port = static_cast<unsigned short>(
        load_server_port("../config/config.ini")
    );
    tcp::acceptor acceptor(ioc, {tcp::v4(), port});
    tcp::socket socket(ioc);

    // Объявляем обработчик с явным типом
    std::function<void(beast::error_code)> accept_handler;

    // Определяем обработчик
    accept_handler = [&](beast::error_code ec) {
      if (!ec) {
        std::make_shared<HttpConnection>(std::move(socket), db)->start();
      }

      // Рекурсивный вызов с захватом по значению
      acceptor.async_accept(socket, accept_handler);
    };

    // Первоначальный вызов
    acceptor.async_accept(socket, accept_handler);

    std::cout << "Server started on port " << port << "\n";
    ioc.run();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}