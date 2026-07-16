#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <boost/asio.hpp>
#include "const.h"
#include <queue>
#include <mutex>

class CServer;

class MsgNode {
	friend class CSession;
public:
	MsgNode(char* msg, int max_len) : _total_len(max_len + HEAD_LENGTH), _cur_len(0) {
		_data = new char[_total_len + 1]();

		//转化为网络字节序
		int max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
		memcpy(_data, &max_len_host, HEAD_LENGTH);
		memcpy(_data + HEAD_LENGTH, msg, max_len);
		_data[_total_len] = '\0';
	}
	

	MsgNode(int max_len) : _total_len(max_len), _cur_len(0) {
		_data = new char[_total_len + 1]();
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear() {
		_cur_len = 0;
		memset(_data, '\0', _total_len);
	}

private:
	char* _data;
	int _total_len;
	int _cur_len;
};

class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);

	std::string& GetUuid();

	boost::asio::ip::tcp::socket& GetSocket();

	void Start();

	void Send(char* msg, int len);

	void Send(std::string& msg);

	void Close();

	std::shared_ptr<CSession> SharedSelf();

private:
	void HandleRead(const boost::system::error_code& error, std::size_t bytes_tranferred, std::shared_ptr<CSession> shared_self);
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

	boost::asio::ip::tcp::socket _socket;
	CServer* _server;

	std::string _uuid;
	bool isClosed;

	char _data[MAX_LENGTH];
	std::queue<std::shared_ptr<MsgNode>> _send_que;
	std::mutex _send_lock;
	
	std::shared_ptr<MsgNode> _recv_head_node;
	std::shared_ptr<MsgNode> _recv_msg_node;
	bool isHeadParsed;
};

