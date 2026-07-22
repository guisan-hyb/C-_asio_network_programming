#include "CServer.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"

int main() {
	try {
		auto& pool = AsioIOServicePool::GetInstance();//处理连接后的收发信息
		boost::asio::io_context ioc;//接收连接
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&pool, &ioc](auto, auto) {
			ioc.stop();
			pool.Stop();
		});

		CServer s(ioc, 10086);
		ioc.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}