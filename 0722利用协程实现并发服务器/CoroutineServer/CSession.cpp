#include "CSession.h"
#include "CServer.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include "const.h"
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _io_context(ioc), _socket(ioc), _server(server), _is_close(false)
{
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<RecvNode>(HEAD_TOTAL_LEN, 0);
}

CSession::~CSession()
{
	try
	{
		std::cout << "CSession destruct" << std::endl;
		Close();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

boost::asio::ip::tcp::socket& CSession::GetSocket()
{
	return _socket;
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

void CSession::Start()
{
	auto shared_this = shared_from_this();
	//开启协程接收
	boost::asio::co_spawn(_io_context, [=]()->boost::asio::awaitable<void> {
		try {
			for (; !_is_close;) {
				_recv_head_node->Clear();
				std::size_t n = co_await boost::asio::async_read(_socket, boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
					boost::asio::use_awaitable);

				if (n == 0) {
					std::cout << "receive peer closed" << std::endl;
					Close();
					_server->ClearSession(_uuid);
					co_return;
				}

				//获取头部MSGID数据
				short msg_id = 0;
				memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
				//网络字节序转化为本地
				msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
				std::cout << "msg id is: " << msg_id << std::endl;

				if (msg_id > MAX_LENGTH) {
					std::cout << "invalid msg id is: " << msg_id << std::endl;
					Close();
					_server->ClearSession(_uuid);
					co_return;
				}

				//获取头部MSGLEN数据
				short msg_len = 0;
				memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
				msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
				std::cout << "msg len is: " << msg_len << std::endl;

				if (msg_len > MAX_LENGTH) {
					std::cout << "invalid msg len is: " << msg_len << std::endl;
					Close();
					_server->ClearSession(_uuid);
					co_return;
				}

				_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);

				//读出包体
				n = co_await boost::asio::async_read(_socket, boost::asio::buffer(_recv_msg_node->_data, _recv_msg_node->_total_len),
					boost::asio::use_awaitable);

				if (n == 0) {
					std::cout << "receive peer closed" << std::endl;
					Close();
					_server->ClearSession(_uuid);
					co_return;
				}

				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				std::cout << "receive data is: " << _recv_msg_node->_data << std::endl;

				//投递给逻辑线程
				LogicSystem::GetInstance().PostMsgToQue(std::make_shared<LogicNode>(shared_this, _recv_msg_node));
			}
		}
		catch (std::exception& e) {
			std::cerr << "Exception: " << e.what() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}, boost::asio::detached);
}

void CSession::Close() {
	_is_close = true;
	_socket.close();
}

void CSession::Send(std::string& msg, short msg_id)
{
	std::unique_lock<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is: " << MAX_SENDQUE << std::endl;
		return;
	}
	_send_que.push(std::make_shared<SendNode>(msg.data(), msg.length(), msg_id));
	if (send_que_size > 0) {
		return;
	}

	auto msgnode = _send_que.front();
	lock.unlock();//只有在使用队列的时候加锁
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_from_this()));
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> session)
{
	try {
		if (!error) {
			std::unique_lock<std::mutex> lock(_send_lock);
			_send_que.pop();
			if (!_send_que.empty()) {
				auto msgnode = _send_que.front();
				lock.unlock();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, session));
			}
		}
		else {
			std::cerr << "handle write failed, error is: " << error.what() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		Close();
		_server->ClearSession(_uuid);
	}
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode)
	: _session(session), _recvnode(recvnode)
{
}
