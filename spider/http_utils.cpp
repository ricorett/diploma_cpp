#include "http_utils.hpp"
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/regex.hpp>
#include <iostream>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

std::pair<std::string, std::string> download(const std::string &url) {
    int redirect_count = 0;
    std::string current_url = url;
    
    while (redirect_count < 5) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);

            auto parsed_url = parse_url(current_url);
            auto results = resolver.resolve(parsed_url.host, parsed_url.port);

            bool is_https = (parsed_url.scheme == "https");

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

                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res);

                beast::error_code ec;
                beast::get_lowest_layer(stream).socket().shutdown(tcp::socket::shutdown_both, ec);

                // Handle redirects
                if (res.result() == http::status::moved_permanently || 
                    res.result() == http::status::found) {
                    if (auto location = res.find(http::field::location); location != res.end()) {
                        current_url = std::string(location->value()); 
                        redirect_count++;
                        continue;
                    }
                }

                if (res.result() != http::status::ok) {
                    std::cerr << "HTTP error: " << res.result() << " for " << current_url << std::endl;
                    return {current_url, ""};
                }

                return {current_url, res.body()};
            } else {
                beast::tcp_stream stream(ioc);
                stream.connect(results);

                http::request<http::string_body> req{http::verb::get, parsed_url.path, 11};
                req.set(http::field::host, parsed_url.host);
                req.set(http::field::user_agent, "Mozilla/5.0 (compatible; SearchEngineSpider/1.0)");
                req.set(http::field::connection, "close");

                http::write(stream, req);

                beast::flat_buffer buffer;
                http::response<http::string_body> res;
                http::read(stream, buffer, res);

                beast::error_code ec;
                stream.socket().shutdown(tcp::socket::shutdown_both, ec);

                // Handle redirects for HTTP
                if (res.result() == http::status::moved_permanently || 
                    res.result() == http::status::found) {
                    if (auto location = res.find(http::field::location); location != res.end()) {
                        current_url = std::string(location->value());
                        redirect_count++;
                        continue;
                    }
                }

                if (res.result() != http::status::ok) {
                    std::cerr << "HTTP error: " << res.result() << " for " << current_url << std::endl;
                    return {current_url, ""};
                }

                return {current_url, res.body()};
            }
        } catch (const std::exception &e) {
            std::cerr << "Download error: " << e.what() << " for " << current_url << std::endl;
            return {current_url, ""};
        }
    }
    
    std::cerr << "Too many redirects for: " << url << std::endl;
    return {current_url, ""};
}

ParsedUrl parse_url(const std::string &url) {
    ParsedUrl result;
    static const boost::regex re(R"(^(https?)://([^/:]+)(?::(\d+))?(/.*)?$)");
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

        // Set default ports
        if (result.scheme == "https" && result.port.empty()) {
            result.port = "443";
        } else if (result.scheme == "http" && result.port.empty()) {
            result.port = "80";
        }
    } else {
        // Fallback for invalid URLs
        result.scheme = "http";
        result.host = url;
        result.port = "80";
        result.path = "/";
    }
    return result;
}