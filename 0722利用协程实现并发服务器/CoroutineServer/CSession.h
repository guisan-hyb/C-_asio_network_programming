#pragma once

#include <boost/asio.hpp>
#include <string>
#include <queue>
#include <mutex>
#include "MsgNode.h"
#include <memory>

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);
	~CSession();

	boost::asio::ip::tcp::socket& GetSocket();
	std::string& GetUuid();

	void Start();
	void Close();
	void Send(std::string& msg, short msg_id);

private:
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> session);

	boost::asio::io_context& _io_context;
	boost::asio::ip::tcp::socket _socket;
	CServer* _server;
	std::string _uuid;
	bool _is_close;

	std::mutex _send_lock;
	std::queue<std::shared_ptr<SendNode>> _send_que;

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