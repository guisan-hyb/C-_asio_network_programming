#pragma once

#include "Singleton.h"
#include <boost/asio.hpp>
#include <thread>
#include <vector>

class AsioThreadPool: public Singleton<AsioThreadPool>
{
	friend class Singleton<AsioThreadPool>;
public:
	using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

	~AsioThreadPool();
	AsioThreadPool(const AsioThreadPool&) = delete;
	AsioThreadPool& operator=(const AsioThreadPool&) = delete;

	boost::asio::io_context& GetIOService();
	void Stop();

private:
	AsioThreadPool(std::size_t size = std::thread::hardware_concurrency());
	boost::asio::io_context _service;
	std::unique_ptr<WorkGuard> _work;
	std::vector<std::thread> _threads;
};

