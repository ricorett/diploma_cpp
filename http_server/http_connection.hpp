#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include "../common/ini_parser.hpp"

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
  HttpConnection(tcp::socket socket, IniParser &config);
  void start();

private:
  tcp::socket                        socket_;
  beast::flat_buffer                 buffer_{8192};
  http::request<http::dynamic_body>  request_;
  http::response<http::dynamic_body> response_;
  IniParser                         &config_;

  void readRequest();
  void processRequest();
  void writeResponse();
  void createResponse();
  void handleSearchRequest();
  void handlePostSearch();
  void performSearch(const std::string &query);
  void serveHomePage();
  void serveStaticFile(const std::string &path);
};