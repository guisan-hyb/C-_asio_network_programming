#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <thread>

class AsioIOServicePool
{
public:
	using IOService = boost::asio::io_context;
	using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
	using WorkGuardPtr = std::unique_ptr<WorkGuard>;

	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

	boost::asio::io_context& GetIOService();
	void Stop();
	static AsioIOServicePool& GetInstance();

private:
	AsioIOServicePool(std::size_t = std::thread::hardware_concurrency());
	std::vector<IOService> _ioServices;
	std::vector<WorkGuardPtr> _works;
	std::vector<std::thread> _threads;
	std::size_t _nextIOService;
};


