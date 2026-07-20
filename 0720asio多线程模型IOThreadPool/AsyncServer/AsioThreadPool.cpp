#include "AsioThreadPool.h"

AsioThreadPool::~AsioThreadPool()
{
	Stop();
}

boost::asio::io_context& AsioThreadPool::GetIOService()
{
	return _service;
}

void AsioThreadPool::Stop()
{
	_work.reset();
	for (auto& t : _threads) {
		t.join();
	}
}

AsioThreadPool::AsioThreadPool(std::size_t threadNum)
	: _work(std::make_unique<WorkGuard>(boost::asio::make_work_guard(_service)))
{
	for (int i = 0; i < threadNum; i++) {
		_threads.emplace_back([this]() {
			_service.run();
		});
	}
}
