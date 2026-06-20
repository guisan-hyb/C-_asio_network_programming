#include "Session.h"
#include <cstring>
#include <iostream>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

Session::Session(boost::asio::io_context& ioc, Server* server) : _socket(ioc), _server(server)
{
	memset(_data, '\0', max_length);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
}

boost::asio::ip::tcp::socket& Session::Socket()
{
	return _socket;
}

void Session::Start()
{
	_socket.async_read_some(boost::asio::buffer(_data, max_length),
		std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2,
			shared_from_this()));

	//注意：这里最后一个参数能写 std::shared_ptr<Session>(this) 吗？
	//不能！这么写十分危险！
	//因为会导致两个智能指针指向同一块内存空间，可能会重析构！（类似浅拷贝）
	//（传递的智能指针和this构造的智能指针不是同一个，引用计数不会共用，但是内存存储会共用）
}

std::string& Session::GetUuid()
{
	return _uuid;
}

void Session::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
	std::shared_ptr<Session> _self_shared)
{
	if (!error) {
		std::cout << "server receive data is: " << _data << std::endl;
		boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transferred),
			std::bind(&Session::handle_write, this, std::placeholders::_1, _self_shared));
	}
	else {
		std::cout << "read error" << std::endl;
		_server->ClearSession(_uuid);
	}
}

void Session::handle_write(const boost::system::error_code& error, std::shared_ptr<Session> _self_shared)
{
	if (!error) {
		memset(_data, '\0', max_length);
		_socket.async_read_some(boost::asio::buffer(_data, max_length),
			std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2, _self_shared));
	}
	else {
		std::cout << "write error" << std::endl;
		_server->ClearSession(_uuid);
	}
}
