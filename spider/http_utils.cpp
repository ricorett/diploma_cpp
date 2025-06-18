#include "http_utils.hpp"
#include <boost/regex.hpp>
#include <iostream>

std::string download(const std::string &url) {
  try {
    net::io_context ioc;
    tcp::resolver   resolver(ioc);

    auto parsed_url = parse_url(url);
    auto results    = resolver.resolve(parsed_url.host, parsed_url.port);

    bool is_https = url.substr(0, 8) == "https://";

    if (is_https) {

      ssl::context ctx(ssl::context::tlsv12_client);
      ctx.set_default_verify_paths();
      ctx.set_verify_mode(ssl::verify_none);

      beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

      if (!SSL_set_tlsext_host_name(stream.native_handle(), parsed_url.host.c_str())) {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
      }

      beast::get_lowest_layer(stream).connect(results);
      stream.handshake(ssl::stream_base::client);

      http::request<http::string_body> req{http::verb::get, parsed_url.path, 11};
      req.set(http::field::host, parsed_url.host);
      req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SearchEngineSpider/1.0)");
      req.set(http::field::connection, "close");

      http::write(stream, req);

      beast::flat_buffer                buffer;
      http::response<http::string_body> res;
      http::read(stream, buffer, res);

      beast::error_code ec;
      beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_both, ec);

      if (res.result() != http::status::ok) {
        std::cerr << "HTTP error: " << res.result() << " for " << url << std::endl;
        return "";
      }

      return res.body();
    } else {

      beast::tcp_stream stream(ioc);
      stream.connect(results);

      http::request<http::string_body> req{http::verb::get, parsed_url.path, 11};
      req.set(http::field::host, parsed_url.host);
      req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SearchEngineSpider/1.0)");
      req.set(http::field::connection, "close");

      http::write(stream, req);

      beast::flat_buffer                buffer;
      http::response<http::string_body> res;
      http::read(stream, buffer, res);

      beast::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);

      if (res.result() != http::status::ok) {
        std::cerr << "HTTP error: " << res.result() << " for " << url << std::endl;
        return "";
      }

      return res.body();
    }
  } catch (const std::exception &e) {
    std::cerr << "Download error: " << e.what() << std::endl;
    return "";
  }
}

ParsedUrl parse_url(const std::string &url) {
  ParsedUrl                 result;
  static const boost::regex re(R"(^(https?)://([^/:]+)(?::(\d+))?(/.*)?$)");
  boost::smatch             match;

  if (boost::regex_match(url, match, re)) {
    result.host = match[2].str();
    if (match[3].matched)
      result.port = match[3].str();
    if (match[4].matched)
      result.path = match[4].str();

    if (match[1] == "https" && result.port == "80") {
      result.port = "443";
    }
  }
  return result;
}