#include "http_connection.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8(const std::string& str) {
	std::string url_decoded = url_decode(str);
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket, Database& database)
        : socket_(std::move(socket)), db(database) {}


void HttpConnection::start()
{
	readRequest();
	checkDeadline();
}


void HttpConnection::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}


void HttpConnection::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void HttpConnection::createResponsePost() {
    if (request_.target() == "/") {
        std::string requestBody = buffers_to_string(request_.body().data());

        size_t pos = requestBody.find('=');
        if (pos == std::string::npos) {
            response_.result(http::status::bad_request);
            beast::ostream(response_.body()) << "Invalid search request";
            return;
        }

        std::string query = requestBody.substr(pos + 1);
        std::vector<std::string> words = extractWordsFromHtml(query);

        if (words.empty()) {
            response_.result(http::status::bad_request);
            beast::ostream(response_.body()) << "No valid words in search query";
            return;
        }

        std::vector<std::pair<std::string, int>> searchResults = db.searchDocuments(words);

        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
                << "<html>\n"
                << "<head><meta charset=\"UTF-8\"><title>Search Results</title></head>\n"
                << "<body>\n"
                << "<h1>Search Results</h1>\n";

        if (searchResults.empty()) {
            beast::ostream(response_.body()) << "<p>No results found.</p>\n";
        } else {
            beast::ostream(response_.body()) << "<ul>\n";
            for (const auto& [url, relevance] : searchResults) {
                beast::ostream(response_.body())
                        << "<li><a href=\"" << url << "\">" << url << "</a> (Relevance: " << relevance << ")</li>\n";
            }
            beast::ostream(response_.body()) << "</ul>\n";
        }

        beast::ostream(response_.body()) << "</body>\n</html>\n";
    } else {
        response_.result(http::status::not_found);
        beast::ostream(response_.body()) << "Page not found";
    }
}

void HttpConnection::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}


