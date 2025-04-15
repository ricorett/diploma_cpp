//#include <Windows.h>
#include <iostream>
#include "http_connection.h"

int main()
{
//	SetConsoleCP(CP_UTF8);
//	SetConsoleOutputCP(CP_UTF8);
//    Database db("/Users/lana/Desktop/work/netology/diploma/Base/database/config.ini");





	try
	{
        Database db;
		auto const address = net::ip::make_address("0.0.0.0");
		unsigned short port = 8080;

		net::io_context ioc{1};

		tcp::acceptor acceptor{ioc, { address, port }};
		tcp::socket socket{ioc};
        httpServer(acceptor, socket, db);

		std::cout << "Open browser and connect to http://localhost:8080 to see the web server operating" << std::endl;

		ioc.run();
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
