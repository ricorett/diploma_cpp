#include "http_server.hpp"
#include "../common/ini_parser.hpp"
#include <boost/asio.hpp>
#include <iostream>

int main(int argc,char** argv) {
    try {
        if(argc < 2){
            throw std::runtime_error("Need path to config");
        }

        IniParser config(argv[1]);
        int port = config.getServerPort();
        

        net::io_context ioc;
        tcp::endpoint endpoint{tcp::v4(), static_cast<unsigned short>(port)};
        HttpServer server(ioc, endpoint);
        
        std::cout << "HTTP server listening on port " << port << std::endl;
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}