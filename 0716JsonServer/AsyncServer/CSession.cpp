#include "CSession.h"
#include "CServer.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _socket(ioc), _server(server), isClosed(false), isHeadParsed(false)
{
	memset(_data, '\0', MAX_LENGTH);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<MsgNode>(HEAD_LENGTH);
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

boost::asio::ip::tcp::socket& CSession::GetSocket()
{
	return _socket;
}

void CSession::Start()
{
	memset(_data, '\0', MAX_LENGTH);
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
		std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
			SharedSelf()));
}

void CSession::Send(char* msg, int len)
{
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	_send_que.push(std::make_shared<MsgNode>(msg, len));
	if (send_que_size > 0) return;

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Send(std::string& msg)
{
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	_send_que.push(std::make_shared<MsgNode>(msg.data(), msg.length()));
	if (send_que_size > 0) return;

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close() {
	isClosed = true;
	_socket.close();
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
	return std::shared_ptr<CSession>();
}

void CSession::HandleRead(const boost::system::error_code& error, std::size_t bytes_tranferred, std::shared_ptr<CSession> shared_self)
{
	try {
		if (!error) {
			int copy_len = 0;
			while (bytes_tranferred > 0) {
				if (!isHeadParsed) {
					if (_recv_head_node->_cur_len + bytes_tranferred < HEAD_LENGTH) {
						memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_tranferred);
						_recv_head_node->_cur_len += bytes_tranferred;
						memset(_data, '\0', MAX_LENGTH);
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
							std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
								shared_self));
						return;
					}

					int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data, head_remain);
					_recv_head_node->_cur_len += head_remain;
					bytes_tranferred -= head_remain;
					copy_len += head_remain;

					short data_len = 0;
					memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
					//网络字节序转化为本地字节序
					data_len = boost::asio::detail::socket_ops::network_to_host_short(data_len);

					if (data_len > MAX_LENGTH) {
						std::cout << "invalid data length is: " << data_len << std::endl;
						_server->ClearSession(_uuid);
						return;
					}

					_recv_msg_node = std::make_shared<MsgNode>(data_len);

					if (bytes_tranferred < data_len) {
						memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_tranferred);
						_recv_msg_node->_cur_len += bytes_tranferred;
						memset(_data, '\0', MAX_LENGTH);
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
							std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
								SharedSelf()));

						isHeadParsed = true;
						return;
					}

					memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
					_recv_msg_node->_cur_len += data_len;
					copy_len + data_len;
					bytes_tranferred -= data_len;
					_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
					//std::cout << "receive data is: " << _recv_msg_node->_data << std::endl;

					std::string raw_str = std::string(_recv_msg_node->_data, _recv_msg_node->_total_len);
					json root = json::parse(raw_str);

					std::cout << "receive msg id is " << root["id"].get<int>() <<
						" msg data is " << root["data"].get<std::string>() << std::endl;

					root["data"] = " server has received msg, msg data is " + root["data"].get<std::string>();
					std::string return_str = root.dump();

					Send(return_str);


					isHeadParsed = false;
					_recv_head_node->Clear();
					if (bytes_tranferred <= 0) {
						memset(_data, '\0', MAX_LENGTH);
						_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
							std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
								SharedSelf()));
						return;
					}

					continue;
				}

				int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
				if (bytes_tranferred < remain_msg) {
					memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_tranferred);
					_recv_msg_node->_cur_len += bytes_tranferred;
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));
					return;
				}

				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
				_recv_msg_node->_cur_len += remain_msg;
				copy_len += remain_msg;
				bytes_tranferred -= remain_msg;
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				//std::cout << "receive data is " << _recv_msg_node->_data << std::endl;

				std::string raw_str(_recv_msg_node->_data, _recv_msg_node->_total_len);
				json root = json::parse(raw_str);

				std::cout << "receive msg id is: " << root["id"].get<int>() <<
					" msg data is: " << root["data"].get<std::string>() << std::endl;

				root["data"] = " server has received msg, msg data is " + root["data"].get<std::string>();
				std::string return_str = root.dump();

				Send(return_str);


				isHeadParsed = false;
				_recv_head_node->Clear();
				if (bytes_tranferred <= 0) {
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::HandleRead, this, std::placeholders::_1, std::placeholders::_2,
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
	catch (std::exception& e) {
		std::cerr << "Exception code: " << e.what() << std::endl;
	}
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self)
{
	try {
		if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			//std::cout << "send data is: " << _send_que.front()->_data + HEAD_LENGTH << std::endl;
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
			}
		}
		else {
			std::cout << "handle write failed, error code is: " << error.message() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code: " << e.what() << std::endl;
	}
}
