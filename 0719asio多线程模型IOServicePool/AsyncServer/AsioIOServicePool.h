#pragma once

#include "Singleton.h"
#include <boost/asio.hpp>
#include <vector>

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
	friend class Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using WorkGuardPtr = std::unique_ptr<WorkGuard>;

	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	//使用 round-robin 的方式返回一个 io_context
	boost::asio::io_context& GetIOService();
	void Stop();

private:
	AsioIOServicePool(std::size_t = std::thread::hardware_concurrency());
	std::vector<IOService> _ioServices;
	std::vector<WorkGuardPtr> _works;
	std::vector<std::thread> _threads;
	std::size_t _nextIOService;
};

