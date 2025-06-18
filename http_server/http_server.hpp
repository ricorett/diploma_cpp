#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace net = boost::asio;
using tcp     = net::ip::tcp;

class HttpServer {
public:
  HttpServer(net::io_context &ioc, tcp::endpoint endpoint);
  void run();

private:
  net::io_context &ioc_;
  tcp::acceptor    acceptor_;

  void doAccept();
  void onAccept(boost::beast::error_code ec, tcp::socket socket); // Добавили boost::
};