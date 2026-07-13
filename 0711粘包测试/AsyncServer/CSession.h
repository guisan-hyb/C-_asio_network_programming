#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <string>
#include <mutex>

#define MAX_LENGTH 1024*2
#define HEAD_LENGTH 2

class CServer;

class MsgNode {
	friend class CSession;
public:
	MsgNode(char* msg, short max_len) : _total_len(max_len + HEAD_LENGTH), _cur_len(0) { //∑¢ÀÕ
		_data = new char[_total_len + 1];
		memcpy(_data, &max_len, HEAD_LENGTH);
		memcpy(_data + HEAD_LENGTH, msg, max_len);
		_data[_total_len] = '\0';
	}

	MsgNode(short max_len) : _total_len(max_len), _cur_len(0) { //Ω” ’
		_data = new char[_total_len + 1]();
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear() {
		memset(_data, '\0', _total_len);
		_cur_len = 0;
	}

private:
	char* _data;
	short _cur_len;
	short _total_len;
};


class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);

	~CSession();

	boost::asio::ip::tcp::socket& Socket();

	std::string& GetUuid();

	void Start();

	std::shared_ptr<CSession> SharedSelf();

	void Close();

	void Send(char* msg, int max_length);


	void PrintRecvData(char* data, int length);

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);
	void handle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared);

	boost::asio::ip::tcp::socket _socket;
	CServer* _server;
	std::string _uuid;
	bool is_close;

	std::queue<std::shared_ptr<MsgNode>> _send_que;
	std::mutex _send_lock;

	char _data[MAX_LENGTH];
	std::shared_ptr<MsgNode> _recv_msg_node;
	std::shared_ptr<MsgNode> _recv_head_node;
	bool is_head_parse;
};

