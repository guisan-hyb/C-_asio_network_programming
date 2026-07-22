#include "AsioIOServicePool.h"
#include <iostream>

AsioIOServicePool::AsioIOServicePool(std::size_t sz)
	: _ioServices(sz), _works(sz), _nextIOService(0) {
	for (std::size_t i = 0; i < sz; i++) {
		_works[i] = std::make_unique<WorkGuard>(boost::asio::make_work_guard(_ioServices[i]));
	}

	//遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
	for (std::size_t i = 0; i < _ioServices.size(); i++) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
		});
	}
}

AsioIOServicePool::~AsioIOServicePool()
{
	Stop(); // 析构时确保线程安全退出
	std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService()
{
	return _ioServices[(_nextIOService++) % _ioServices.size()];
}

void AsioIOServicePool::Stop()
{
	for (auto& ioc : _ioServices) {
		ioc.stop();
	}

	for (auto& work : _works) {
		work.reset();
	}

	for (auto& t : _threads) {
		if (t.joinable()) {
			t.join();
		}
	}
}

AsioIOServicePool& AsioIOServicePool::GetInstance()
{
	static AsioIOServicePool instance;
	return instance;
}
