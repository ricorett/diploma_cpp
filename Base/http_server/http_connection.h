#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <memory>
#include "../database/database.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
 explicit HttpConnection(tcp::socket socket, Database& db);
 void start();

private:
 void read_request();
 void process_request();
 void create_get_response();
 void create_post_response();
 void write_response();
 void check_timeout();

 tcp::socket socket_;
 beast::flat_buffer buffer_{8192};
 http::request<http::dynamic_body> request_;
 http::response<http::dynamic_body> response_;
 net::steady_timer timer_{socket_.get_executor(), std::chrono::seconds(60)};
 Database& db_;
};