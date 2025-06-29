#include "http_connection.hpp"
#include "../database/database.hpp"
#include "ini_parser.hpp"
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>
#include <sstream>

bool ends_with(const std::string &str, const std::string &suffix) {
  return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

// Функция для декодирования URL-encoded строк
std::string urlDecode(const std::string &str) {
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '+') {
      result += ' ';
    } else if (str[i] == '%' && i + 2 < str.size()) {
      int                hex_val;
      std::istringstream hex_stream(str.substr(i + 1, 2));
      hex_stream >> std::hex >> hex_val;
      result += static_cast<char>(hex_val);
      i += 2;
    } else {
      result += str[i];
    }
  }
  return result;
}

// Функция для очистки UTF-8 строк
std::string cleanUtf8(const std::string &input) {
  std::string output;
  output.reserve(input.size());

  for (char c : input) {
    // Оставляем только валидные символы
    if ((c >= 32 && c <= 126) || (static_cast<unsigned char>(c) >= 0xC0)) {
      output.push_back(c);
    } else {
      // Заменяем невалидные символы на пробел
      output.push_back(' ');
    }
  }
  return output;
}

HttpConnection::HttpConnection(tcp::socket socket, IniParser &config)
    : socket_(std::move(socket))
    , config_(config) {
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
  // Обработка поисковых запросов
  if (request_.method() == http::verb::get && request_.target().starts_with("/search")) {

    handleSearchRequest();
  }
  // Обработка главной страницы
  else if (request_.method() == http::verb::get) {
    serveHomePage();
  }
  // Обработка POST запросов (поиск)
  else if (request_.method() == http::verb::post) {
    handlePostSearch();
  }
  // Неподдерживаемые методы
  else {
    response_.result(http::status::bad_request);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Invalid request method\n";
    writeResponse();
  }
}

void HttpConnection::handleSearchRequest() {
  std::string target = std::string(request_.target());
  size_t      pos    = target.find("q=");

  if (pos != std::string::npos) {
    std::string query = target.substr(pos + 2);
    query             = urlDecode(query);
    query             = cleanUtf8(query);

    performSearch(query);
  } else {
    response_.result(http::status::bad_request);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Missing query parameter";
    writeResponse();
  }
}

void HttpConnection::handlePostSearch() {
  // Get the raw POST data
  std::string post_data = beast::buffers_to_string(request_.body().data());

  // Debug output (remove in production)
  std::cerr << "Received POST data: " << post_data << std::endl;

  // Parse URL-encoded parameters
  std::unordered_map<std::string, std::string> params;
  std::istringstream                           param_stream(post_data);
  std::string                                  token;

  while (std::getline(param_stream, token, '&')) {
    size_t eq_pos = token.find('=');
    if (eq_pos != std::string::npos) {
      std::string key   = token.substr(0, eq_pos);
      std::string value = token.substr(eq_pos + 1);
      params[key]       = urlDecode(value);
    }
  }

  // Check for query parameter
  if (params.find("q") != params.end()) {
    std::string query = cleanUtf8(params["q"]);
    performSearch(query);
  } else {
    response_.result(http::status::bad_request);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Missing query parameter 'q' in POST data";
    writeResponse();
  }
}

void HttpConnection::performSearch(const std::string &query) {
  try {
    // Подключение к БД с паролем
    std::string conn_str = "dbname=" + config_.getDbName() +
                           " "
                           "user=" +
                           config_.getDbUser() +
                           " "
                           "password=" +
                           config_.getDbPassword() +
                           " "
                           "host=" +
                           config_.getDbHost() +
                           " "
                           "port=" +
                           config_.getDbPort();
    pqxx::connection c{conn_str};

    // Безопасный запрос с параметрами
    const char *sql = R"(
      SELECT d.url 
      FROM documents d 
      JOIN document_word dw ON d.id = dw.document_id 
      JOIN words w ON w.id = dw.word_id 
      WHERE w.word ILIKE $1 
      LIMIT 10
    )";

    pqxx::work   txn{c};
    pqxx::result r = txn.exec_params(std::string_view(sql), "%" + query + "%");

    std::ifstream     template_file("../web/results.html");
    std::stringstream template_buffer;
    template_buffer << template_file.rdbuf();
    std::string html = template_buffer.str();

    // Replace placeholders
    size_t pos;

    // Replace query
    pos = html.find("{{query}}");
    if (pos != std::string::npos) {
      html.replace(pos, 9, query);
    }

    // Replace results
    std::string results_html;
    if (r.empty()) {
      results_html = "<p class='no-results'>No results found</p>";
    } else {
      results_html = "<ul>";
      for (auto row : r) {
        std::string url = row["url"].as<std::string>();
        results_html += "<li class='result'>";
        results_html += "<div class='title'><a href=\"" + url + "\">" + url + "</a></div>";
        results_html += "<div class='url'>" + url + "</div>";
        results_html += "</li>";
      }
      results_html += "</ul>";
    }

    pos = html.find("{{results}}");
    if (pos != std::string::npos) {
      html.replace(pos, 11, results_html);
    }

    // Replace pagination (simplified)
    pos = html.find("{{pagination}}");
    if (pos != std::string::npos) {
      html.replace(pos, 14, "<span>Page 1 of 1</span>");
    }

    response_.result(http::status::ok);
    response_.set(http::field::content_type, "text/html");
    beast::ostream(response_.body()) << html;

  } catch (const std::exception &e) {
    response_.result(http::status::internal_server_error);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Database error: " << e.what();
  }

  writeResponse();
}

void HttpConnection::serveHomePage() {
  std::string target = std::string(request_.target());

  // Отдаем index.html для корня и других GET запросов
  if (target == "/" || target == "/index.html") {
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
  // Отдаем другие статические файлы (CSS, JS)
  else {
    serveStaticFile(target);
  }

  writeResponse();
}

void HttpConnection::serveStaticFile(const std::string &path) {
  // Безопасное извлечение пути к файлу
  std::string safe_path = path;
  if (safe_path.find("..") != std::string::npos) {
    response_.result(http::status::bad_request);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "Invalid path";
    return;
  }

  std::string   full_path = "../web" + safe_path;
  std::ifstream file(full_path, std::ios::binary);

  if (file) {
    std::stringstream buffer;
    buffer << file.rdbuf();

    response_.result(http::status::ok);

    if (ends_with(safe_path, ".css")) {
      response_.set(http::field::content_type, "text/css");
    } else if (ends_with(safe_path, ".js")) {
      response_.set(http::field::content_type, "application/javascript");
    } else if (ends_with(safe_path, ".png")) {
      response_.set(http::field::content_type, "image/png");
    } else {
      response_.set(http::field::content_type, "text/plain");
    }

    beast::ostream(response_.body()) << buffer.str();
  } else {
    response_.result(http::status::not_found);
    response_.set(http::field::content_type, "text/plain");
    beast::ostream(response_.body()) << "File not found: " << safe_path;
  }
}

void HttpConnection::writeResponse() {
  auto self = shared_from_this();

  response_.content_length(response_.body().size());
  response_.keep_alive(request_.keep_alive());

  http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t) {
    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
  });
}