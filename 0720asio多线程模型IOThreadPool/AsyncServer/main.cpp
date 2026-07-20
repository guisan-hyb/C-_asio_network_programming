#define _CRT_SECURE_NO_WARNINGS

#include "AsioThreadPool.h"
#include <iostream>
#include "CServer.h"
#include <csignal>

int main() {
	try {
		auto pool = AsioThreadPool::GetInstance();
		boost::asio::io_context ioc;
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([pool, &ioc](auto, auto) {
			ioc.stop();
			pool->Stop();
		});

		CServer s(pool->GetIOService(), 10086);

		ioc.run();
		std::cout << "server exited " << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}
