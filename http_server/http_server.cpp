#include "http_server.hpp"
#include "http_connection.hpp"
#include <iostream>

HttpServer::HttpServer(net::io_context &ioc, tcp::endpoint endpoint, const IniParser &config)
    : ioc_(ioc)
    , acceptor_(ioc, endpoint)
    , config_(config) {
  run();
}

void HttpServer::run() {
  doAccept();
}

void HttpServer::doAccept() {
  acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
    if (!ec) {
      std::make_shared<HttpConnection>(std::move(socket),config_)->start();
    } else {
      std::cerr << "Accept error: " << ec.message() << std::endl;
    }
    doAccept();
  });
}