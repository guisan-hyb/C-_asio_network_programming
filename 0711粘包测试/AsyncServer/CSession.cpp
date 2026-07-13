#include "CSession.h"
#include "CServer.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <chrono>
#include <thread>

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _socket(ioc), _server(server), is_close(false), is_head_parse(false)
{
	memset(_data, '\0', MAX_LENGTH);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<MsgNode>(HEAD_LENGTH);
}

CSession::~CSession()
{
	std::cout << "CSession destructed" << std::endl;
}

boost::asio::ip::tcp::socket& CSession::Socket()
{
	return _socket;
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

void CSession::Start()
{
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
		std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2, SharedSelf()));
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
	return shared_from_this();
}

void CSession::Close() {
	is_close = true;
	_socket.close();
}

void CSession::Send(char* msg, int max_length)
{
	bool isWorking = false;
	std::lock_guard<std::mutex> lock(_send_lock);
	if (_send_que.size() > 0) isWorking = true;
	_send_que.push(std::make_shared<MsgNode>(msg, max_length));
	
	if (isWorking) return;

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::handle_write, this, std::placeholders::_1, SharedSelf()));
}

void CSession::PrintRecvData(char* data, int length)
{
	std::stringstream ss;
	std::string result("0x");
	for (int i = 0; i < length; i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(data[i]));
	}
	result += ss.str();
	std::cout << "Received Data: " << result << std::endl;
}

void CSession::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<CSession> _self_shared)
{
	if (!error) {
		PrintRecvData(_data, bytes_transferred);
		std::chrono::milliseconds dura(2000);
		std::this_thread::sleep_for(dura);

		int copy_len = 0;
		while (bytes_transferred > 0) {
			if (!is_head_parse) {
				if (bytes_transferred + _recv_head_node->_cur_len < HEAD_LENGTH) {
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transferred);
					_recv_head_node->_cur_len += bytes_transferred;
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2, SharedSelf()));
					return;
				}

				int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
				memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);
				copy_len += head_remain;
				bytes_transferred -= head_remain;

				short data_len = 0;
				memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
				std::cout << "data_len is " << data_len << std::endl;

				if (data_len > MAX_LENGTH) {
					std::cout << "invalid data length is: " << data_len << std::endl;
					_server->ClearSession(_uuid);
					Close();
					return;
				}

				_recv_msg_node = std::make_shared<MsgNode>(data_len);

				if (bytes_transferred < data_len) {
					memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
					_recv_msg_node->_cur_len += bytes_transferred;
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));

					is_head_parse = true;
					return;
				}

				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
				_recv_msg_node->_cur_len += data_len;
				copy_len += data_len;
				bytes_transferred -= data_len;
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';

				std::cout << "receive data is " << _recv_msg_node->_data << std::endl;
				Send(_recv_msg_node->_data, _recv_msg_node->_total_len);

				is_head_parse = false;
				_recv_head_node->Clear();
				if (bytes_transferred <= 0) {
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));
					return;
				}

				continue;
			}

			int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
			if (bytes_transferred < remain_msg) {
				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
				_recv_msg_node->_cur_len += bytes_transferred;
				memset(_data, '\0', MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
						SharedSelf()));
				return;
			}

			memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
			_recv_msg_node->_cur_len += remain_msg;
			copy_len += remain_msg;
			bytes_transferred -= remain_msg;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			std::cout << "receive data is " << _recv_msg_node->_data << std::endl;
			Send(_recv_msg_node->_data, _recv_msg_node->_total_len);

			is_head_parse = false;
			_recv_head_node->Clear();
			if (bytes_transferred <= 0) {
				memset(_data, '\0', MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
						SharedSelf()));
				return;
			}

			continue;
		}
	}
	else {
		std::cout << "handle read failed, error code is: " << error.message() << std::endl;
		Close();
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
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
				std::bind(&CSession::handle_write, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed, error code is: " << error.message() << std::endl;
		Close();
		_server->ClearSession(_uuid);
	}
}

