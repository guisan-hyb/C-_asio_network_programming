#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "CServer.h"
#include <csignal>


int main() {
	try {
		boost::asio::io_context ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&ioc](auto, auto) {
			ioc.stop();
		});

		CServer server(ioc, 10086);
		ioc.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}