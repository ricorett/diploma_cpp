#include "http_utils.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/regex.hpp>
#include <unordered_set>
#include <chrono>
#include <iostream>
#include <thread>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// Функция для корректного разрешения относительных URL
static std::string resolve_url(const std::string &base,
                               const std::string &location) {
  // Абсолютный URL
  if (location.find("://") != std::string::npos) {
    return location;
  }

  // URL относительно схемы (//example.com/path)
  if (location.size() >= 2 && location[0] == '/' && location[1] == '/') {
    ParsedUrl base_parsed = parse_url(base);
    return base_parsed.scheme + ":" + location;
  }

  ParsedUrl base_parsed = parse_url(base);
  std::string result;

  // Абсолютный путь (/path)
  if (location[0] == '/') {
    result = base_parsed.scheme + "://" + base_parsed.host;
    if (base_parsed.port != "80" && base_parsed.port != "443") {
      result += ":" + base_parsed.port;
    }
    result += location;
    return result;
  }

  // Относительный путь (path)
  result = base_parsed.scheme + "://" + base_parsed.host;
  if (base_parsed.port != "80" && base_parsed.port != "443") {
    result += ":" + base_parsed.port;
  }

  // Находим последний слэш в базовом пути
  size_t last_slash = base_parsed.path.find_last_of('/');
  if (last_slash == std::string::npos) {
    result += "/" + location;
  } else {
    result += base_parsed.path.substr(0, last_slash + 1) + location;
  }
  return result;
}

// Функция для замены HTML-сущностей в URL
std::string decodeUrlEntities(const std::string &url) {
  std::string decoded = url;
  boost::replace_all(decoded, "&amp;", "&");
  boost::replace_all(decoded, "&quot;", "\"");
  boost::replace_all(decoded, "&lt;", "<");
  boost::replace_all(decoded, "&gt;", ">");
  return decoded;
}

// Функция для проверки поддерживаемых схем
bool isSupportedScheme(const std::string &scheme) {
  static const std::unordered_set<std::string> supported = {"http", "https"};
  return supported.find(scheme) != supported.end();
}

std::pair<std::string, std::string> download(const std::string &url) {
  int redirect_count = 0;
  std::string current_url = url;

  while (redirect_count < 5) {
    try {
      // Декодируем HTML-сущности в URL
      current_url = decodeUrlEntities(current_url);

      net::io_context ioc;
      tcp::resolver resolver(ioc);

      auto parsed_url = parse_url(current_url);

      // Проверяем поддерживаемые схемы
      if (!isSupportedScheme(parsed_url.scheme)) {
        std::cerr << "Skipping unsupported scheme: " << parsed_url.scheme
                  << " for URL: " << current_url << std::endl;
        return {current_url, ""};
      }

      auto results = resolver.resolve(parsed_url.host, parsed_url.port);

      bool is_https = (parsed_url.scheme == "https");

      // Устанавливаем таймауты
      constexpr auto connect_timeout = std::chrono::seconds(10);
      constexpr auto operation_timeout = std::chrono::seconds(30);

      if (is_https) {
        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_none);

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Устанавливаем таймаут на соединение
        beast::get_lowest_layer(stream).expires_after(connect_timeout);

        if (!SSL_set_tlsext_host_name(stream.native_handle(),
                                      parsed_url.host.c_str())) {
          beast::error_code ec{static_cast<int>(::ERR_get_error()),
                               net::error::get_ssl_category()};
          throw beast::system_error{ec};
        }

        beast::get_lowest_layer(stream).connect(results);

        // Устанавливаем таймаут на операции
        beast::get_lowest_layer(stream).expires_after(operation_timeout);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, parsed_url.path,
                                             11};
        req.set(http::field::host, parsed_url.host);
        req.set(http::field::user_agent,
                "Mozilla/5.0 (compatible; SearchEngineSpider/1.0)");
        req.set(http::field::connection, "close");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        beast::get_lowest_layer(stream).socket().shutdown(
            tcp::socket::shutdown_both, ec);

        // Обработка редиректов с разрешением URL
        if (res.result() == http::status::moved_permanently ||
            res.result() == http::status::found ||
            res.result() == http::status::see_other) { // Добавляем 303

          if (auto location = res.find(http::field::location);
              location != res.end()) {
            current_url =
                resolve_url(current_url, std::string(location->value()));
            redirect_count++;
            continue;
          }
        }

        if (res.result() != http::status::ok) {
          std::cerr << "HTTP error: " << res.result() << " for " << current_url
                    << std::endl;
          return {current_url, ""};
        }

        return {current_url, res.body()};
      } else {
        beast::tcp_stream stream(ioc);

        // Устанавливаем таймаут на соединение
        stream.expires_after(connect_timeout);
        stream.connect(results);

        // Устанавливаем таймаут на операции
        stream.expires_after(operation_timeout);

        http::request<http::string_body> req{http::verb::get, parsed_url.path,
                                             11};
        req.set(http::field::host, parsed_url.host);
        req.set(http::field::user_agent,
                "Mozilla/5.0 (compatible; SearchEngineSpider/1.0)");
        req.set(http::field::connection, "close");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // Обработка редиректов с разрешением URL
        if (res.result() == http::status::moved_permanently ||
            res.result() == http::status::found ||
            res.result() == http::status::see_other) { // Добавляем 303

          if (auto location = res.find(http::field::location);
              location != res.end()) {
            current_url =
                resolve_url(current_url, std::string(location->value()));
            redirect_count++;
            continue;
          }
        }

        if (res.result() != http::status::ok) {
          std::cerr << "HTTP error: " << res.result() << " for " << current_url
                    << std::endl;
          return {current_url, ""};
        }

        return {current_url, res.body()};
      }
    } catch (const beast::system_error &e) {
      // Обработка временных ошибок с повторными попытками
      if (e.code() == boost::asio::error::connection_refused ||
          e.code() == boost::asio::error::timed_out) {

        static int retry_count = 0;
        constexpr int max_retries = 2;

        if (retry_count < max_retries) {
          retry_count++;
          std::this_thread::sleep_for(std::chrono::seconds(1));
          continue; // Повторная попытка
        }
      }

      std::cerr << "Download error: " << e.what() << " for " << current_url
                << std::endl;
      return {current_url, ""};
    } catch (const std::exception &e) {
      std::cerr << "Download error: " << e.what() << " for " << current_url
                << std::endl;
      return {current_url, ""};
    }
  }

  std::cerr << "Too many redirects for: " << url << std::endl;
  return {current_url, ""};
}

ParsedUrl parse_url(const std::string &url) {
  ParsedUrl result;
  static const boost::regex re(R"(^([a-z]+):\/\/([^/:]+)(?::(\d+))?(\/.*)?$)",
                               boost::regex::icase);
  boost::smatch match;

  if (boost::regex_match(url, match, re)) {
    result.scheme = match[1].str();
    result.host = match[2].str();

    if (match[3].matched) {
      result.port = match[3].str();
    }

    if (match[4].matched) {
      result.path = match[4].str();
    } else {
      result.path = "/";
    }

    // Установка портов по умолчанию
    if (result.scheme == "https" && result.port.empty()) {
      result.port = "443";
    } else if (result.scheme == "http" && result.port.empty()) {
      result.port = "80";
    }
  } else {
    // Возвращаем unsupported scheme для некорректных URL
    result.scheme = "unsupported";
    result.host = "";
    result.port = "";
    result.path = "";
  }
  return result;
}