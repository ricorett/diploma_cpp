#include "http_connection.h"
#include "../spider/html_parser.h"
#include <boost/beast/version.hpp>
#include <sstream>

HttpConnection::HttpConnection(tcp::socket socket, Database& db)
    : socket_(std::move(socket)), db_(db) {}

void HttpConnection::start() {
    read_request();
    check_timeout();
}

void HttpConnection::read_request() {
    auto self = shared_from_this();
    http::async_read(socket_, buffer_, request_,
        [self](beast::error_code ec, size_t) {
            if (!ec) self->process_request();
        });
}

void HttpConnection::process_request() {
    response_.version(request_.version());
    response_.keep_alive(false);

    switch(request_.method()) {
    case http::verb::get:
        create_get_response();
        break;
    case http::verb::post:
        create_post_response();
        break;
    default:
        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "Unsupported method";
        break;
    }
    write_response();
}

void HttpConnection::create_get_response() {
    if (request_.target() == "/") {
        response_.result(http::status::ok);
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body())
            << "<html><head><title>Search Engine</title></head>"
            << "<body><h1>Search</h1>"
            << "<form method='post'>"
            << "<input name='query' type='text'>"
            << "<input type='submit'></form></body></html>";
    } else {
        response_.result(http::status::not_found);
        beast::ostream(response_.body()) << "404 Not Found";
    }
}

void HttpConnection::create_post_response() {
    std::string body = beast::buffers_to_string(request_.body().data());
    size_t pos = body.find("query=");
    if (pos == std::string::npos) {
        response_.result(http::status::bad_request);
        beast::ostream(response_.body()) << "Invalid request";
        return;
    }

    std::string query = body.substr(pos + 6);
    auto words = extract_words(query);

    if (words.empty()) {
        response_.result(http::status::bad_request);
        beast::ostream(response_.body()) << "No valid search terms";
        return;
    }

    auto results = db_.search(words);
    response_.set(http::field::content_type, "text/html");
    beast::ostream(response_.body())
        << "<html><head><title>Results</title></head><body>"
        << "<h1>Found " << results.size() << " results:</h1><ul>";

    for (const auto& [url, score] : results) {
        beast::ostream(response_.body())
            << "<li><a href=\"" << url << "\">" << url
            << "</a> (" << score << ")</li>";
    }
    beast::ostream(response_.body()) << "</ul></body></html>";
}

void HttpConnection::write_response() {
    auto self = shared_from_this();
    response_.content_length(response_.body().size());
    http::async_write(socket_, response_,
        [self](beast::error_code ec, size_t) {
            self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            self->timer_.cancel();
        });
}

void HttpConnection::check_timeout() {
    auto self = shared_from_this();
    timer_.async_wait([self](beast::error_code ec) {
        if (!ec) self->socket_.close(ec);
    });
}