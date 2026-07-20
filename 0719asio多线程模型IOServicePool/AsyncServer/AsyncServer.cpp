#define _CRT_SECURE_NO_WARNINGS


#include <iostream>
#include "CServer.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"

bool b_stop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

void sig_handler(int sig) {
	if (sig == SIGINT || sig == SIGTERM) {
		std::unique_lock<std::mutex> lock_quit(mutex_quit);
		b_stop = true;
		cond_quit.notify_one();
	}
}

int main() {
	try {
		auto pool = AsioIOServicePool::GetInstance();

		boost::asio::io_context ioc;
		std::thread	net_work_thread([&ioc]() {
			CServer server(ioc, 10086);
			ioc.run();
		});

		signal(SIGINT, sig_handler);
		signal(SIGTERM, sig_handler);

		while (!b_stop) {
			std::unique_lock<std::mutex> lock_quit(mutex_quit);
			cond_quit.wait(lock_quit);
		}

		ioc.stop();
		net_work_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}

