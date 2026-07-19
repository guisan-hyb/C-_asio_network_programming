#pragma once

#include <boost/asio.hpp>
#include <string>
#include "MsgNode.h"
#include "const.h"
#include <memory>
#include <queue>
#include <mutex>

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);

	boost::asio::ip::tcp::socket& GetSocket();

	std::string& GetUuid();

	std::shared_ptr<CSession> SharedSelf();

	void Start();

	void Send(char* msg, short len, short msg_id);

	void Send(std::string& msg, short msg_id);

	void Close();

private:
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared);
	void HandleReadHead(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);
	void HandleReadMsg(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);

	boost::asio::ip::tcp::socket _socket;
	std::string _uuid;
	CServer* _server;

	bool isClose;

	std::queue<std::shared_ptr<SendNode>> _send_que;
	std::mutex _send_lock;

	std::shared_ptr<RecvNode> _recv_head_node;
	std::shared_ptr<RecvNode> _recv_msg_node;
};



class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode);

private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recvnode;
};