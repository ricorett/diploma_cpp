#include "http_connection.hpp"
#include "../database/database.hpp"
#include <fstream>
#include <pqxx/pqxx>
#include <sstream>

HttpConnection::HttpConnection(tcp::socket socket)
    : socket_(std::move(socket)) {
}

void HttpConnection::start() {
  readRequest();
}

void HttpConnection::readRequest() {
  auto self = shared_from_this();

  http::async_read(socket_, buffer_, request_, [self](beast::error_code ec, std::size_t bytes_transferred) {
    if (!ec) {
      self->processRequest();
    }
  });
}

void HttpConnection::processRequest() {
  if (request_.method() == http::verb::get) {
    std::string target = std::string(request_.target());

    if (target.find("/search") == 0) {
      size_t pos = target.find("q=");
      if (pos != std::string::npos) {
        std::string query    = target.substr(pos + 2);
        size_t      plus_pos = 0;
        while ((plus_pos = query.find("+", plus_pos)) != std::string::npos) {
          query.replace(plus_pos, 1, " ");
          plus_pos++;
        }

        std::string search_query = "SELECT d.url FROM documents d JOIN document_word dw ON d.id = dw.document_id JOIN words w ON w.id = "
                                   "dw.word_id WHERE w.word ILIKE '%" +
                                   query + "%' LIMIT 10;";

        try {
          pqxx::connection c{"dbname=search_engine user=user123 hostaddr=127.0.0.1 port=5432"};
          pqxx::work       txn{c};
          pqxx::result     r = txn.exec(search_query);

          std::stringstream html;
          html << "<h1>Search Results for: " << query << "</h1><ul>";
          for (auto row : r) {
            html << "<li><a href='" << row["url"].c_str() << "'>" << row["url"].c_str() << "</a></li>";
          }
          html << "</ul><br><a href='/'>Back to search</a>";

          response_.result(http::status::ok);
          response_.set(http::field::content_type, "text/html");
          beast::ostream(response_.body()) << html.str();
        } catch (const std::exception &e) {
          response_.result(http::status::internal_server_error);
          response_.set(http::field::content_type, "text/plain");
          beast::ostream(response_.body()) << "Database error: " << e.what();
        }
      } else {
        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "Missing query parameter";
      }
    } else {
      std::ifstream file("../web/index.html");
      if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        response_.result(http::status::ok);
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body()) << buffer.str();
      } else {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\n";
      }
    }
  } else if (request_.method() == http::verb::post) {
    std::string post_data = beast::buffers_to_string(request_.body().data());

    std::string query;
    size_t      pos = post_data.find("query=");
    if (pos != std::string::npos) {
      query          = post_data.substr(pos + 6);
      size_t end_pos = query.find("&");
      if (end_pos != std::string::npos) {
        query = query.substr(0, end_pos);
      }
      size_t plus_pos = 0;
      while ((plus_pos = query.find("+", plus_pos)) != std::string::npos) {
        query.replace(plus_pos, 1, " ");
        plus_pos++;
      }
    }

    std::string search_query = "SELECT d.url FROM documents d JOIN document_word dw ON d.id = dw.document_id JOIN words w ON w.id = "
                               "dw.word_id WHERE w.word ILIKE '%" +
                               query + "%' LIMIT 5;";

    try {
      pqxx::connection c{"dbname=search_engine user=user123 hostaddr=127.0.0.1 port=5432"};
      pqxx::work       txn{c};
      pqxx::result     r = txn.exec(search_query);

      std::stringstream html;
      html << "<h1>Search Results</h1><ul>";
      for (auto row : r) {
        html << "<li><a href='" << row["url"].c_str() << "'>" << row["url"].c_str() << "</a></li>";
      }
      html << "</ul>";

      response_.result(http::status::ok);
      response_.set(http::field::content_type, "text/html");
      beast::ostream(response_.body()) << html.str();

    } catch (const std::exception &e) {
      response_.result(http::status::internal_server_error);
      response_.set(http::field::content_type, "text/plain");
      beast::ostream(response_.body()) << "Database error: " << e.what();
    }
  } else {
    response_.result(http::status::bad_request);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Invalid request method\n";
  }

  writeResponse();
}

void HttpConnection::writeResponse() {
  auto self = shared_from_this();

  response_.content_length(response_.body().size());
  response_.keep_alive(request_.keep_alive());

  http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t) {
    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
  });
}