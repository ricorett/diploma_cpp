#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <codecvt>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>

#include "../spider/html_parser.h"
#include "../database/database.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
protected:

	tcp::socket socket_;

	beast::flat_buffer buffer_{8192};

	http::request<http::dynamic_body> request_;

	http::response<http::dynamic_body> response_;


	net::steady_timer deadline_{
		socket_.get_executor(), std::chrono::seconds(60)};

	void readRequest();
	void processRequest();

	void createResponseGet();

	void createResponsePost();
	void writeResponse();
	void checkDeadline();

public:
	HttpConnection(tcp::socket socket, Database& database);
	void start();

private:
    Database& db;
};

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, Database& db);