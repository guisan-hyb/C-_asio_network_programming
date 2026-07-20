#include "CSession.h"
#include <iostream>
#include "CServer.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include "LogicSystem.h"

using json = nlohmann::json;

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _socket(ioc),_server(server), isClose(false), _strand(ioc.get_executor())
{
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);

	_recv_head_node = std::make_shared<RecvNode>(HEAD_TOTAL_LEN, 0);
}

boost::asio::ip::tcp::socket& CSession::GetSocket()
{
	return _socket;
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
	return shared_from_this();
}

void CSession::Start()
{
	_recv_head_node->Clear();
	boost::asio::async_read(_socket, boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
		boost::asio::bind_executor(_strand,
			std::bind(&CSession::HandleReadHead, this, std::placeholders::_1, std::placeholders::_2,
			SharedSelf()))
		);
}

void CSession::Send(char* msg, short len, short msg_id)
{
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << "send que fulled, size is" << MAX_SENDQUE << std::endl;
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg, len, msg_id));
	if (send_que_size > 0) return;

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		boost::asio::bind_executor(_strand,
			std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()))
		);
}

void CSession::Send(std::string& msg, short msg_id)
{
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << "send que fulled, size is" << MAX_SENDQUE << std::endl;
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg.data(), msg.length(), msg_id));
	if (send_que_size > 0) return;

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		boost::asio::bind_executor(_strand,
			std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()))
		);
}

void CSession::Close() {
	isClose = true;
	_socket.close();
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared)
{
	try {
		if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					boost::asio::bind_executor(_strand,
						std::bind(&CSession::HandleWrite, this, std::placeholders::_1, _self_shared))
					);
			}
		}
		else {
			std::cout << "handle write failed, error code is: " << error.message() << std::endl;
			_server->ClearSession(_uuid);
			Close();
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code: " << error.what() << std::endl;
	}
}

void CSession::HandleReadHead(const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<CSession> _self_shared)
{
	try {
		if (!error) {
			if (bytes_transferred < HEAD_TOTAL_LEN) {
				std::cout << "read head length error" << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}

			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			if (msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is: " << msg_id << std::endl;
				_server->ClearSession(_uuid);
				return;
			}

			short msg_len = 0;
			memcpy(&msg_len , _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid msg_length is: " << msg_len << std::endl;
				_server->ClearSession(_uuid);
				return;
			}

			_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
			boost::asio::async_read(_socket, boost::asio::buffer(_recv_msg_node->_data, _recv_msg_node->_total_len),
				boost::asio::bind_executor(_strand,
					std::bind(&CSession::HandleReadMsg, this, std::placeholders::_1, std::placeholders::_2,
					_self_shared)
				));
		}
		else {
			std::cout << "headle read head failed, error: " << error.message() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code: " << error.message() << std::endl;
	}
}

void CSession::HandleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<CSession> _self_shared)
{
	try {
		if (!error) {
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';

			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(_self_shared, _recv_msg_node));

			_recv_head_node->Clear();
			boost::asio::async_read(_socket, boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
				boost::asio::bind_executor(_strand,
					std::bind(&CSession::HandleReadHead, this, std::placeholders::_1, std::placeholders::_2,
					_self_shared))
				);
		}
		else {
			std::cout << "headle read msg failed, error: " << error.message() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code: " << error.what() << std::endl;
	}
}



LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	: _session(session), _recvnode(recvnode)
{
}
