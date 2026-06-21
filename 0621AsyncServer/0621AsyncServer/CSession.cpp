#include "CSession.h"

#include <cstring>
#include <iostream>
#include "CServer.h"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _socket(ioc), _server(server)
{
	memset(_data, '\0', max_length);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
}

boost::asio::ip::tcp::socket& CSession::Socket()
{
	return _socket;
}

void CSession::Start()
{
	_socket.async_read_some(boost::asio::buffer(_data, max_length),
		std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
			shared_from_this()));
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

void CSession::Send(char* msg, int max_length)
{
	bool isPending = false;//是否还有未发送完的数据
	std::lock_guard<std::mutex> lock(_send_lock);
	if (_send_que.size() > 0) {
		isPending = true;
	}

	_send_que.push(std::make_shared<MsgNode>(msg, max_length));

	if (isPending) return;

	boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
		std::bind(&CSession::handle_write, this, std::placeholders::_1, shared_from_this()));
}


void CSession::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<CSession> _self_shared)
{
	if (!error) {
		std::cout << "read data is" << _data << std::endl;

		Send(_data, bytes_transferred);//发送数据

		memset(_data, '\0', bytes_transferred);
		_socket.async_read_some(boost::asio::buffer(_data, max_length),
			std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
				_self_shared));
	}
	else {
		std::cout << "handle read failed, error is: " << error.what() << std::endl;
		_server->ClearSession(_uuid);
	}
}

void CSession::handle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared)
{
	if (!error) {
		std::lock_guard<std::mutex> lock(_send_lock);
		_send_que.pop();
		if (!_send_que.empty()) {
			auto& msgnode = _send_que.front();
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_max_len),
				std::bind(&CSession::handle_write, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed, error is: " << error.what() << std::endl;
		_server->ClearSession(_uuid);
	}
}
